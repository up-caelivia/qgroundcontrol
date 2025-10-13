import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

Item {
    id: root
    width: 900; height: 560
    clip: true

    // === INPUT (lat,lon,alt,label opzionale) ===
    property var gpsPoints: [
        { lat: 41.89031, lon: 12.49223, alt:  2,  label: "A" },
        { lat: 41.89031, lon: 12.49223, alt: 10,  label: "B" },
        { lat: 41.89045, lon: 12.49210, alt: 10,  label: "C" },
        { lat: 41.89045, lon: 12.49210, alt:  5,  label: "D" },
        { lat: 41.89045, lon: 12.49210, alt:  2,  label: "E" },
        { lat: 41.89039, lon: 12.49223, alt:  2,  label: "F" },
        { lat: 41.89036, lon: 12.49221, alt:  2,  label: "G" }
    ]

    // === PARAMETRI VISTA ===
    readonly property real elevDeg: 10
    readonly property real alphaDeg: _alphaDeg
    property real _alphaDeg: 0

    // φ discreto (gradi)
    property int phiDeg: 90

    // origine fissa nel sistema X'Y' (dopo la rotazione su α)
    property real x0: 0     // min X'
    property real y0: 0     // mean Y'

    // pan/zoom
    property real zoom: 1.0
    property real panX: 0.0
    property real panY: 0.0

    // stile
    property color gridXZ: "#30ffd15a"
    property color gridYZ: "#308f7af0"
    property color axesColor: "#000000"
    property color pointColor: "#000000"
    property color labelColor: "#000000"
    property int   labelFontPx: 12

    // === precisione: niente sotto 1 cm ===
    readonly property real _CM: 0.01
    function q(v) { return Math.round(v/_CM)*_CM }
    function fmtMeters(v) {
        var n = Number(v);
        if (!isFinite(n)) return "";
        return Number(n).toFixed(2);
    }

    // --- POLIGONO ---
    property bool   showPolygon: true
    property color  polygonStroke: "#222222"
    property color  polygonFill:   "#2088c0ff"
    property real   polygonWidth:  2
    property string polygonOrder: "input"
    property var    polygonLabels: [] // es. ["A","F","C","B","D","E"]

    // --- MESH ---
    property bool  showMesh: true
    property real  meshStep: 2.0
    property string meshStartLabel: "E"
    property string meshEndLabel:   "A"
    property color meshColor: "#f0c000"
    property real  meshWidth: 1.5
    property string meshMode: "tri"
    property real meshStepX: 2.0
    property real meshStepZ: 2.0
    property bool meshDrawVertical: true
    property bool meshDrawHorizontal: true
    property real meshSnakeStepZ: 2.0
    property real meshSnakeX: 0
    property bool meshSnakeAutoX: true
    property real meshGridStepU: 2.0
    property real meshGridStepV: 2.0
    property bool meshTriShowDiagonals: true
    property color meshTriColor: "#f0c000"

    // --- SCAN PATH (disegno) ---
    property bool  showScanPath: true
    property color scanPathColor: "#ff0066"
    property real  scanPathWidth: 2.5
    property bool  scanStartFromLeft: true
    property var   scanPathPoints: []   // ENU, per disegno 2D

    // --- SWEEP (serpentina parallela a un bordo) ---
    property string sweepStartLabel: "C"     // se vuoti → usa il lato più lungo
    property string sweepEndLabel:   "B"
    property real   sweepStep: 0.50
    property bool   sweepStartFromMinU: true
    property bool   sweepFromLeft: true     // solo per disegno ausiliario
    property var    redPoints: []

    // === OUTPUT missione: path in LLA ===
    property var    scanPathLLA: []         // [{lat,lon,alt}]

    // === Stato della conversione ENU↔LLA ===
    property var  refLLA: ({lat:0,lon:0,alt:0})  // fissato in rebuild()
    property real zOffsetENU: 0                    // minZ tolto in rebuild()

    // === Geo helpers ===
    function deg2rad(d){ return d*Math.PI/180.0 }
    function geodeticToECEF(lat, lon, h) {
        var a=6378137.0, f=1.0/298.257223563, e2=f*(2-f)
        var phi=deg2rad(lat), lam=deg2rad(lon), sinp=Math.sin(phi), cosp=Math.cos(phi)
        var sinl=Math.sin(lam), cosl=Math.cos(lam)
        var N= a / Math.sqrt(1 - e2*sinp*sinp)
        var x=(N+h)*cosp*cosl
        var y=(N+h)*cosp*sinl
        var z=(N*(1-e2)+h)*sinp
        return {x:x,y:y,z:z}
    }
    function ecefToENU(x, y, z, refLLA) {
        var a=geodeticToECEF(refLLA.lat, refLLA.lon, refLLA.alt)
        var dx=x-a.x, dy=y-a.y, dz=z-a.z
        var phi=deg2rad(refLLA.lat), lam=deg2rad(refLLA.lon)
        var sinp=Math.sin(phi), cosp=Math.cos(phi), sinl=Math.sin(lam), cosl=Math.cos(lam)
        var e = -sinl*dx +  cosl*dy
        var n = -sinp*cosl*dx - sinp*sinl*dy + cosp*dz
        var u =  cosp*cosl*dx +  cosp*sinl*dy + sinp*dz
        return {x:e, y:n, z:u}
    }

    // sin/cos esatti per angoli discreti
    function sincosDeg(d) {
        var a = ((d % 360) + 360) % 360
        switch (a) {
        case 0:   return {c: 1, s: 0}
        case 30:  return {c: Math.sqrt(3)/2, s: 0.5}
        case 60:  return {c: 0.5, s: Math.sqrt(3)/2}
        case 90:  return {c: 0, s: 1}
        case 180: return {c: -1, s: 0}
        case 270: return {c: 0, s: -1}
        default:
            var r = deg2rad(a)
            return {c: Math.cos(r), s: Math.sin(r)}
        }
    }

    // === Proiezione (φ ruota X′Y′, elevazione = elevDeg) ===
    function projectXZ(p3prime) {
        var th = deg2rad(elevDeg), c = Math.cos(th), s = Math.sin(th)
        var sc = sincosDeg(phiDeg); var cp = sc.c, sp = sc.s
        var eps = 1e-12
        if (Math.abs(cp) < eps) cp = 0
        if (Math.abs(sp) < eps) sp = 0

        var X = p3prime.x - x0
        var Y = p3prime.y - y0
        var horiz =  c * ( X*cp - Y*sp )
        var vert  =  p3prime.z + s * ( X*sp + Y*cp )

        return { x: q(horiz), y: q(vert), label: p3prime.label }
    }

    // === Dati derivati ===
    property var enuRaw: []       // [{x,y,z,label}]
    property var enuRot: []       // [{x,y,z,label}]
    property var projPoints: []   // [{x,y,label}]
    property var bounds2D: ({minx:0,maxx:1,miny:0,maxy:1})

    // helper poligono
    function indexByLabel(lab) {
        for (var i=0;i<enuRot.length;i++) if (enuRot[i].label===lab) return i
        return -1
    }
    function convexHullIndices() {
        if (enuRot.length < 3) { var tiny=[]; for (var t=0;t<enuRot.length;t++) tiny.push(t); return tiny }
        var pts = []
        for (var i=0;i<enuRot.length;i++) pts.push({x:enuRot[i].x, y:enuRot[i].y, i:i})
        pts.sort(function(a,b){ return a.x===b.x ? (a.y-b.y) : (a.x-b.x) })
        function cross(o,a,b){ return (a.x-o.x)*(b.y-o.y) - (a.y-o.y)*(b.x-o.x) }
        var lower=[]
        for (var p=0;p<pts.length;p++){
            while (lower.length>=2 && cross(pts[lower[lower.length-2]], pts[lower[lower.length-1]], pts[p]) <= 0) lower.pop()
            lower.push(p)
        }
        var upper=[]
        for (var q=pts.length-1;q>=0;q--){
            while (upper.length>=2 && cross(pts[upper[upper.length-2]], pts[upper[upper.length-1]], pts[q]) <= 0) upper.pop()
            upper.push(q)
        }
        var hull = lower.slice(0, lower.length-1).concat(upper.slice(0, upper.length-1))
        var out=[]
        for (var k=0; k<hull.length; k++) out.push(pts[hull[k]].i)
        return out
    }
    function polygonIndices() {
        if (polygonLabels && polygonLabels.length>0) {
            var out=[]
            for (var i=0;i<polygonLabels.length;i++){ var idx=indexByLabel(polygonLabels[i]); if (idx>=0) out.push(idx) }
            return out
        }
        if (polygonOrder==="convex") return convexHullIndices()
        var arr=[]; for (var j=0;j<enuRot.length;j++) arr.push(j); return arr
    }

    // ====== Helpers XY per serpentina/mesh ======
    function signedArea2(a,b,c){ return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x) }
    function isCCW(poly){
        var A=0; for (var i=0;i<poly.length;i++){ var p=poly[i], q=poly[(i+1)%poly.length]; A += (q.x-p.x)*(q.y+p.y) }
        return A<0
    }
    function pointInTri(p,a,b,c){
        var v0x=c.x-a.x, v0y=c.y-a.y
        var v1x=b.x-a.x, v1y=b.y-a.y
        var v2x=p.x-a.x, v2y=p.y-a.y
        var dot00=v0x*v0x+v0y*v0y, dot01=v0x*v1x+v0y*v1y, dot02=v0x*v2x+v0y*v2y
        var dot11=v1x*v1x+v1y*v1y, dot12=v1x*v2x+v1y*v2y
        var inv=1/Math.max(1e-12,(dot00*dot11 - dot01*dot01))
        var u=(dot11*dot02 - dot01*dot12)*inv
        var v=(dot00*dot12 - dot01*dot02)*inv
        return u>=-1e-9 && v>=-1e-9 && (u+v)<=1+1e-9
    }
    function earClipTriangulate(polyIn){
        if (polyIn.length<3) return []
        var poly = polyIn.slice()
        if (!isCCW(poly)) poly.reverse()
        var idx=[]; for (var i=0;i<poly.length;i++) idx.push(i)
        var triangles=[]
        var guard=0
        while (idx.length>3 && guard<10000){
            guard++
            var clipped=false
            for (var k=0;k<idx.length;k++){
                var i0=idx[(k-1+idx.length)%idx.length]
                var i1=idx[k]
                var i2=idx[(k+1)%idx.length]
                var a=poly[i0], b=poly[i1], c=poly[i2]
                if (signedArea2(a,b,c) <= 1e-12) continue
                var ok=true
                for (var j=0;j<idx.length;j++){
                    var ii=idx[j]; if (ii===i0||ii===i1||ii===i2) continue
                    if (pointInTri(poly[ii], a,b,c)) { ok=false; break }
                }
                if (!ok) continue
                triangles.push([a,b,c])
                idx.splice(k,1); clipped=true; break
            }
            if (!clipped) break
        }
        if (idx.length===3){ triangles.push([poly[idx[0]], poly[idx[1]], poly[idx[2]]]) }
        return triangles
    }
    function polygon2DOrdered() {
        var idxs = polygonIndices()
        var out = []
        for (var k=0;k<idxs.length;k++)
            out.push(projectXZ( enuRot[idxs[k]] ))
        return out
    }
    function polygonXYOrdered() {
        var idxs = polygonIndices()
        var out = []
        for (var k=0;k<idxs.length;k++){
            var p=enuRot[idxs[k]]
            out.push({x:p.x, y:p.y, i:idxs[k]})
        }
        return out
    }
    function splitChains(startLabel, endLabel) {
        var idxs = polygonIndices()
        var iS=-1, iE=-1
        for (var i=0;i<idxs.length;i++){
            var p = enuRot[idxs[i]]
            if (p.label===startLabel) iS=i
            if (p.label===endLabel)   iE=i
        }
        if (iS<0 || iE<0) return {c1:[], c2:[]}
        var c1=[], k=iS
        while (true){
            c1.push(projectXZ( enuRot[idxs[k]] ))
            if (k===iE) break
            k = (k+1) % idxs.length
        }
        var c2=[], h=iE
        while (true){
            c2.push(projectXZ( enuRot[idxs[h]] ))
            if (h===iS) break
            h = (h+1) % idxs.length
        }
        return {c1:c1, c2:c2}
    }
    function cumLen(poly) {
        var L=[0];
        for (var i=1;i<poly.length;i++){
            var dx=poly[i].x-poly[i-1].x;
            var dy=poly[i].y-poly[i-1].y;
            var seg = Math.sqrt(dx*dx + dy*dy);
            L.push(L[i-1] + seg);
        }
        return L;
    }
    function pointAt(poly, Lc, s) {
        if (poly.length===0) return {x:0,y:0}
        if (s<=0) return poly[0]
        var Ltot = Lc[Lc.length-1]
        if (s>=Ltot) return poly[poly.length-1]
        var i=1
        while (i<Lc.length && Lc[i] <= s) i++
        while (i>1 && Math.abs(Lc[i]-Lc[i-1])<1e-12) i--
        var denom = (Lc[i] - Lc[i-1])
        var t = denom!==0 ? (s - Lc[i-1]) / denom : 0
        return { x: poly[i-1].x + t*(poly[i].x - poly[i-1].x),
                 y: poly[i-1].y + t*(poly[i].y - poly[i-1].y) }
    }

    // ---- Intersezioni per mesh/proiezione ----
    function polyProjectedClosed() {
        var P = polygon2DOrdered()
        if (P.length && (P[0].x!==P[P.length-1].x || P[0].y!==P[P.length-1].y)) P.push({x:P[0].x, y:P[0].y})
        return P
    }
    function verticalCuts(xconst, poly) {
        var eps=1e-9, ys=[]
        for (var i=0;i<poly.length-1;i++){
            var a=poly[i], b=poly[i+1]
            var dx=b.x-a.x, dy=b.y-a.y
            if (Math.abs(dx) < eps) continue
            var ymin=Math.min(a.y,b.y), ymax=Math.max(a.y,b.y)
            if (xconst < Math.min(a.x,b.x)-eps || xconst > Math.max(a.x,b.x)+eps) continue
            var t=(xconst-a.x)/dx
            if (t>-eps && t<1+eps){
                var y=a.y + t*dy
                if (y>ymin+eps && y<=ymax+eps) ys.push(y)
            }
        }
        ys.sort(function(u,v){return u-v})
        var out=[]; for (var k=0;k<ys.length;k++){ if (!out.length || Math.abs(ys[k]-out[out.length-1])>1e-6) out.push(ys[k]) }
        return out
    }
    function horizontalCuts(yconst, poly) {
        var eps=1e-9, xs=[]
        for (var i=0;i<poly.length-1;i++){
            var a=poly[i], b=poly[i+1]
            var dx=b.x-a.x, dy=b.y-a.y
            if (Math.abs(dy) < eps) continue
            var ymin=Math.min(a.y,b.y), ymax=Math.max(a.y,b.y)
            if (yconst < ymin-eps || yconst > ymax+eps) continue
            var t=(yconst-a.y)/dy
            if (t>-eps && t<1+eps){
                var x=a.x + t*dx
                if (x>Math.min(a.x,b.x)-eps && x<=Math.max(a.x,b.x)+eps) xs.push(x)
            }
        }
        xs.sort(function(u,v){return u-v})
        var out=[]; for (var k=0;k<xs.length;k++){ if (!out.length || Math.abs(xs[k]-out[out.length-1])>1e-6) out.push(xs[k]) }
        return out
    }

    // === Sweep XY (piano reale) ===
    function horizontalCutsXY(yconst, poly){
        var eps=1e-9, xs=[]
        for (var i=0;i<poly.length;i++){
            var a=poly[i], b=poly[(i+1)%poly.length]
            var dx=b.x-a.x, dy=b.y-a.y
            if (Math.abs(dy) < eps) continue
            var ymin=Math.min(a.y,b.y), ymax=Math.max(a.y,b.y)
            if (yconst < ymin-eps || yconst > ymax+eps) continue
            var t=(yconst-a.y)/dy
            if (t>-eps && t<1+eps){
                xs.push(a.x + t*dx)
            }
        }
        xs.sort(function(u,v){return u-v})
        var out=[]; for (var k=0;k<xs.length;k++) if (!out.length || Math.abs(xs[k]-out[out.length-1])>1e-6) out.push(xs[k])
        return out
    }
    function earClipTriangulateIdx(polyXY){
        if (polyXY.length<3) return []
        function area2(P){ var A=0; for (var t=0;t<P.length;t++){ var a=P[t], b=P[(t+1)%P.length]; A+=(b.x-a.x)*(b.y+a.y) } return A }
        var P = polyXY.slice()
        if (area2(P)>0) P.reverse() // vogliamo CCW (area2 < 0)
        var idx=[]; for (var i=0;i<P.length;i++) idx.push(i)
        function sa2(a,b,c){ return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x) }
        function pointInTri2(p,a,b,c){
            var v0x=c.x-a.x, v0y=c.y-a.y, v1x=b.x-a.x, v1y=b.y-a.y, v2x=p.x-a.x, v2y=p.y-a.y
            var dot00=v0x*v0x+v0y*v0y, dot01=v0x*v1x+v0y*v1y, dot02=v0x*v2x+v0y*v2y
            var dot11=v1x*v1x+v1y*v1y, dot12=v1x*v2x+v1y*v2y
            var inv=1/Math.max(1e-12,(dot00*dot11 - dot01*dot01))
            var u=(dot11*dot02 - dot01*dot12)*inv, v=(dot00*dot12 - dot01*dot02)*inv
            return u>=-1e-9 && v>=-1e-9 && (u+v)<=1+1e-9
        }
        var tris=[]
        var guard=0
        while (idx.length>3 && guard<10000){
            guard++
            var clipped=false
            for (var k=0;k<idx.length;k++){
                var i0=idx[(k-1+idx.length)%idx.length], i1=idx[k], i2=idx[(k+1)%idx.length]
                var A=P[i0], B=P[i1], C=P[i2]
                if (sa2(A,B,C) <= 1e-12) continue
                var ok=true
                for (var j=0;j<idx.length;j++){
                    var ii=idx[j]; if (ii===i0||ii===i1||ii===i2) continue
                    if (pointInTri2(P[ii],A,B,C)){ ok=false; break }
                }
                if (!ok) continue
                tris.push([i0,i1,i2])
                idx.splice(k,1); clipped=true; break
            }
            if (!clipped) break
        }
        if (idx.length===3) tris.push([idx[0],idx[1],idx[2]])
        var out=[]
        for (var t=0;t<tris.length;t++){
            var a=polyXY[tris[t][0]].i, b=polyXY[tris[t][1]].i, c=polyXY[tris[t][2]].i
            out.push([a,b,c])
        }
        return out
    }
    function interpZAtXY(x, y, triIdxList){
        var p={x:x,y:y}
        for (var t=0;t<triIdxList.length;t++){
            var ia=triIdxList[t][0], ib=triIdxList[t][1], ic=triIdxList[t][2]
            var A={x:enuRot[ia].x,y:enuRot[ia].y}, B={x:enuRot[ib].x,y:enuRot[ib].y}, C={x:enuRot[ic].x,y:enuRot[ic].y}
            if (pointInTri(p,A,B,C)){
                // baricentriche
                var v0x=C.x-A.x, v0y=C.y-A.y, v1x=B.x-A.x, v1y=B.y-A.y, v2x=p.x-A.x, v2y=p.y-A.y
                var d00=v0x*v0x+v0y*v0y, d01=v0x*v1x+v0y*v1y, d11=v1x*v1x+v1y*v1y, d20=v2x*v0x+v2y*v0y, d21=v2x*v1x+v2y*v1y
                var inv=1/Math.max(1e-12,(d00*d11 - d01*d01))
                var u=(d11*d20 - d01*d21)*inv, v=(d00*d21 - d01*d20)*inv, w=1-u-v
                return u*enuRot[ia].z + v*enuRot[ib].z + w*enuRot[ic].z
            }
        }
        // fallback: z media
        var mz=0; for (var i=0;i<enuRot.length;i++) mz+=enuRot[i].z; return mz/(enuRot.length||1)
    }

    // ENU → ECEF → LLA
    function enuToECEF(e, n, u, refLLA){
        var a=geodeticToECEF(refLLA.lat,refLLA.lon,refLLA.alt)
        var phi=deg2rad(refLLA.lat), lam=deg2rad(refLLA.lon)
        var sinp=Math.sin(phi), cosp=Math.cos(phi), sinl=Math.sin(lam), cosl=Math.cos(lam)
        var dx = -sinl*e - sinp*cosl*n + cosp*cosl*u
        var dy =  cosl*e - sinp*sinl*n + cosp*sinl*u
        var dz =             cosp*n     + sinp*u
        return {x:a.x+dx, y:a.y+dy, z:a.z+dz}
    }
    function ecefToGeodetic(x,y,z){
        var a=6378137.0, f=1.0/298.257223563, b=a*(1-f)
        var e2=1-(b*b)/(a*a), ep2=(a*a)/(b*b)-1
        var p=Math.sqrt(x*x+y*y)
        var th=Math.atan2(a*z, b*p)
        var lon=Math.atan2(y,x)
        var lat=Math.atan2(z + ep2*b*Math.pow(Math.sin(th),3),
                           p - e2*a*Math.pow(Math.cos(th),3))
        var N=a/Math.sqrt(1-e2*Math.sin(lat)*Math.sin(lat))
        var alt=p/Math.cos(lat)-N
        return {lat:lat*180/Math.PI, lon:lon*180/Math.PI, alt:alt}
    }

    // Direzione sweep su XY (da due etichette o lato più lungo)
    function sweepDirectionUV_XY() {
        var polyIdx = polygonIndices()
        if (polyIdx.length < 2) return {ux:1, uy:0, p0:{x:0,y:0}}
        var a=null, b=null
        if (sweepStartLabel!=="" && sweepEndLabel!=="") {
            var iA=indexByLabel(sweepStartLabel), iB=indexByLabel(sweepEndLabel)
            if (iA>=0 && iB>=0) { a=enuRot[iA]; b=enuRot[iB]; }
        }
        if (!a) {
            var maxL=-1
            for (var i=0;i<polyIdx.length;i++){
                var p=enuRot[polyIdx[i]], q=enuRot[polyIdx[(i+1)%polyIdx.length]]
                var dx=q.x-p.x, dy=q.y-p.y, L=dx*dx+dy*dy
                if (L>maxL){ maxL=L; a=p; b=q }
            }
        }
        var dx=b.x-a.x, dy=b.y-a.y, L=Math.sqrt(dx*dx+dy*dy)||1
        return {ux:dx/L, uy:dy/L, p0:{x:a.x,y:a.y}}
    }

    // Costruisce il path a serpentina sul piano XY′ e gli z interpolati (ritorna ENU)
    function buildSweepPathENU(step) {
        var polyIdx = polygonIndices()
        if (polyIdx.length < 3) return []

        // ---- direzione u: da etichette (es. "C" -> "B") oppure lato più lungo
        var A0=null, B0=null
        if (sweepStartLabel!=="" && sweepEndLabel!=="") {
            var iA=indexByLabel(sweepStartLabel), iB=indexByLabel(sweepEndLabel)
            if (iA>=0 && iB>=0) { A0=enuRot[iA]; B0=enuRot[iB] }
        }
        if (!A0) {
            var maxL=-1
            for (var i=0;i<polyIdx.length;i++){
                var p=enuRot[polyIdx[i]], q=enuRot[polyIdx[(i+1)%polyIdx.length]]
                var L=(q.x-p.x)*(q.x-p.x) + (q.y-p.y)*(q.y-p.y)
                if (L>maxL){ maxL=L; A0=p; B0=q }
            }
        }
        var ux=B0.x-A0.x, uy=B0.y-A0.y, L=Math.sqrt(ux*ux+uy*uy)||1
        ux/=L; uy/=L
        var nx=-uy, ny=ux
        var P0={x:A0.x,y:A0.y} // origine (sull’edge di partenza)

        // --- poligono XY “chiuso”
        var polyXY = polygonXYOrdered()
        if (polyXY.length && (polyXY[0].x!==polyXY[polyXY.length-1].x || polyXY[0].y!==polyXY[polyXY.length-1].y))
            polyXY.push({x:polyXY[0].x, y:polyXY[0].y, i:polyXY[0].i})

        // --- punto-in-poligono (ray casting) su XY
        function pip(pt){
            var inside=false
            for (var k=0; k+1<polyXY.length; ++k){
                var a=polyXY[k], b=polyXY[k+1]
                var inter = ((a.y>pt.y)!==(b.y>pt.y)) &&
                            (pt.x < (b.x-a.x)*(pt.y-a.y)/(b.y-a.y+1e-18) + a.x)
                if (inter) inside=!inside
            }
            return inside
        }

        // --- scegli il verso della normale che entra nel poligono
        var mid={x:(A0.x+B0.x)/2, y:(A0.y+B0.y)/2}
        var inPlus  = pip({x:mid.x+nx*1e-3, y:mid.y+ny*1e-3})
        var sign = inPlus ? +1 : -1
        nx*=sign; ny*=sign

        // --- trasformazioni UV
        function toUV(p){ var dx=p.x-P0.x, dy=p.y-P0.y; return {u:dx*ux + dy*uy, v:dx*nx + dy*ny} }
        function fromUV(uu,vv){ return { x:P0.x + uu*ux + vv*nx, y:P0.y + uu*uy + vv*ny } }

        var UV=[]; for (var t=0;t<polyXY.length; ++t) UV.push(toUV(polyXY[t]))

        // --- triangoli per Z
        var triIdx = earClipTriangulateIdx(polygonXYOrdered())
        function zAt(x,y){ return interpZAtXY(x,y,triIdx) }

        // --- intersezione con la retta v = costante
        function cutAtV(v){
            var us=[]
            for (var k=0;k+1<UV.length;k++){
                var a=UV[k], b=UV[k+1], dv=b.v-a.v, du=b.u-a.u
                if (Math.abs(dv)<1e-12) continue
                // half-open per evitare duplicati sugli spigoli
                if ((a.v<=v && v<b.v) || (b.v<=v && v<a.v)){
                    var t=(v-a.v)/dv
                    us.push(a.u + t*du)
                }
            }
            us.sort(function(x,y){return x-y})
            var uniq=[]; for (var s=0;s<us.length;s++) if (!uniq.length || Math.abs(us[s]-uniq[uniq.length-1])>1e-9) uniq.push(us[s])
            if (uniq.length>=2) return {u1:uniq[0], u2:uniq[uniq.length-1]}
            return null
        }

        // --- sweep: parte appena DENTRO dal bordo scelto e va verso l’interno
        var st = (step && step>0) ? step
                : (meshSnakeStepZ>0 ? meshSnakeStepZ
                : (meshStep>0 ? meshStep : 0.5))

        // grazie al flip nx,ny sopra, v>0 è verso l’interno: entra di mezza banda
        var v = st * 0.5
        var rows=[]
        var guard=0, MAX=20000

        while (guard++ < MAX) {
            var seg = cutAtV(v)
            if (!seg) break
            var A=fromUV(seg.u1, v), B=fromUV(seg.u2, v)
            rows.push({A:{x:A.x,y:A.y,z:zAt(A.x,A.y)}, B:{x:B.x,y:B.y,z:zAt(B.x,B.y)}})
            v += st
        }


        // --- costruisci il percorso con CONNETTORI alla stessa estremità
        var path=[]
        for (var i=0;i<rows.length;i++){
            var r=rows[i]
            if (i===0){
                // prima riga: A→B
                path.push(r.A); path.push(r.B)
            } else {
                var pr=rows[i-1]
                var swap = (i%2===1) // righe dispari vanno B→A
                if (!swap){
                    // riga pari: A→B; connettore pr.B → r.A
                    path.push({x:r.A.x,y:r.A.y,z:r.A.z})
                    path.push({x:r.B.x,y:r.B.y,z:r.B.z})
                } else {
                    // riga dispari: B→A; connettore pr.B → r.B
                    path.push({x:r.B.x,y:r.B.y,z:r.B.z})
                    path.push({x:r.A.x,y:r.A.y,z:r.A.z})
                }
            }
        }
        return path
    }


    // === UI ===
    Rectangle { anchors.fill: parent; color: "#f7f9fb" }

    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 64; color: "#ffffff"; border.color:"#dddddd"
        Column {
            anchors.fill: parent; anchors.margins: 8; spacing: 6
            Row {
                spacing: 12
                Label { text: "Assonometria ENU — assi su α, elev 10°, rotazione φ discreta, scala in m"; font.bold: true }
                Label { text: "α = " + Number(alphaDeg).toFixed(1) + "°" }
            }
            Row {
                spacing: 16
                Label { text: "φ:" }
                ButtonGroup { id: phiGroup }
                Repeater {
                    model: [0, 30, 60, 90]
                    delegate: RadioButton {
                        text: modelData + "°"
                        checked: phiDeg === modelData
                        onClicked: { phiDeg = modelData; rebuildProjectionOnly() }
                        ButtonGroup.group: phiGroup
                    }
                }
                CheckBox { text: "Poligono"; checked: showPolygon; onToggled: { showPolygon = checked; cv.requestPaint() } }
                ComboBox {
                    model: ["input","convex"]
                    currentIndex: polygonOrder==="convex" ? 1 : 0
                    onCurrentIndexChanged: { polygonOrder = (currentIndex===1 ? "convex" : "input"); cv.requestPaint() }
                }
                CheckBox { text: "Mesh"; checked: showMesh; onToggled: { showMesh = checked; cv.requestPaint() } }
                CheckBox { text: "Percorso"; checked: showScanPath; onToggled: { showScanPath = checked; cv.requestPaint() } }
                Label { text: "Passo (m):" }
                Row {
                    spacing: 6
                    TextField {
                        id: meshStepField
                        text: Number(meshStep).toLocaleString(Qt.locale(), 'f', 2)
                        validator: DoubleValidator { bottom: 0.01; top: 1000; decimals: 2 }
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onEditingFinished: {
                            var v = parseFloat(text.replace(',', '.'))
                            if (!isNaN(v)) { meshStep = Math.max(0.01, v); cv.requestPaint() }
                            text = Number(meshStep).toLocaleString(Qt.locale(), 'f', 2)
                        }
                        width: 70
                    }
                    Button {
                        text: "−"
                        onClicked: { meshStep = Math.max(0.01, meshStep - 0.1); meshStepField.text = Number(meshStep).toLocaleString(Qt.locale(),'f',2); cv.requestPaint() }
                    }
                    Button {
                        text: "+"
                        onClicked: { meshStep = meshStep + 0.1; meshStepField.text = Number(meshStep).toLocaleString(Qt.locale(),'f',2); cv.requestPaint() }
                    }
                }
                Button { text: "Fit"; onClicked: { zoom=1.0; panX=0; panY=0; rebuildProjectionOnly() } }
            }
        }
    }

    Canvas {
        id: cv
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom }
        anchors.topMargin: 64
        antialiasing: true

        function toScreen(p2) {
            var w = width, h = height
            var sx = (p2.x - bounds2D.minx) / (bounds2D.maxx - bounds2D.minx || 1) * w
            var sy = h - (p2.y - bounds2D.miny) / (bounds2D.maxy - bounds2D.miny || 1) * h
            return {x: panX + zoom*sx, y: panY + zoom*sy}
        }

        // step "bello" (m) ~100 px
        function niceStep(pxPerMeter) {
            var targetPx = 100
            var base = [0.01,0.02,0.05,0.1,0.2,0.5,1,2,5,10,20,50,100]
            for (var i=0;i<base.length;i++) {
                var st = base[i]
                var px = st * pxPerMeter
                if (px >= 80 && px <= 140) return st
            }
            return Math.max(0.01, targetPx/pxPerMeter)
        }

        // --- poligono XY con indice nel vettore enuRot
        function polygonXYOrdered() {
            var idxs = polygonIndices()
            var out = []
            for (var k=0;k<idxs.length;k++){
                var p = enuRot[idxs[k]]
                out.push({x:p.x, y:p.y, z:p.z, i: idxs[k]})
            }
            return out
        }

        // Ear clipping → triangoli come indici dentro enuRot
        function earClipTriangulateIdx(polyIn){
            if (polyIn.length<3) return []
            function signedArea2(a,b,c){ return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x) }
            function isCCW(poly){ var A=0; for (var i=0;i<poly.length;i++){var p=poly[i],q=poly[(i+1)%poly.length]; A += (q.x-p.x)*(q.y+p.y)} return A<0 }
            function pointInTri(p,a,b,c){
                var v0x=c.x-a.x, v0y=c.y-a.y, v1x=b.x-a.x, v1y=b.y-a.y, v2x=p.x-a.x, v2y=p.y-a.y
                var dot00=v0x*v0x+v0y*v0y, dot01=v0x*v1x+v0y*v1y, dot02=v0x*v2x+v0y*v2y
                var dot11=v1x*v1x+v1y*v1y, dot12=v1x*v2x+v1y*v2y
                var inv=1/Math.max(1e-12,(dot00*dot11 - dot01*dot01))
                var u=(dot11*dot02 - dot01*dot12)*inv, v=(dot00*dot12 - dot01*dot02)*inv
                return u>=-1e-9 && v>=-1e-9 && (u+v)<=1+1e-9
            }
            var poly = polyIn.slice()
            if (!isCCW(poly)) poly.reverse()
            var idx=[]; for (var i=0;i<poly.length;i++) idx.push(i)
            var tris=[], guard=0
            while (idx.length>3 && guard++<10000){
                var clipped=false
                for (var k=0;k<idx.length;k++){
                    var i0=idx[(k-1+idx.length)%idx.length], i1=idx[k], i2=idx[(k+1)%idx.length]
                    var a=poly[i0], b=poly[i1], c=poly[i2]
                    if (signedArea2(a,b,c) <= 1e-12) continue
                    var ok=true
                    for (var j=0;j<idx.length;j++){
                        var ii=idx[j]; if (ii===i0||ii===i1||ii===i2) continue
                        if (pointInTri(poly[ii], a,b,c)) { ok=false; break }
                    }
                    if (!ok) continue
                    tris.push([a.i, b.i, c.i]) // indici dentro enuRot
                    idx.splice(k,1); clipped=true; break
                }
                if (!clipped) break
            }
            if (idx.length===3){ var a=poly[idx[0]], b=poly[idx[1]], c=poly[idx[2]]; tris.push([a.i,b.i,c.i]) }
            return tris
        }

        // Trasformazione UV parallela al bordo scelto (o lato più lungo), verso “dentro”
        function buildSweepUVFrame(){
            var idxs = polygonIndices(); if (idxs.length<2) return null
            var A0=null, B0=null
            if (sweepStartLabel!=="" && sweepEndLabel!==""){
                var iA=indexByLabel(sweepStartLabel), iB=indexByLabel(sweepEndLabel)
                if (iA>=0 && iB>=0){ A0=enuRot[iA]; B0=enuRot[iB] }
            }
            if (!A0){
                var poly=polygonXYOrdered(), maxL=-1
                for (var i=0;i<poly.length;i++){
                    var p=poly[i], q=poly[(i+1)%poly.length]
                    var L=(q.x-p.x)*(q.x-p.x)+(q.y-p.y)*(q.y-p.y)
                    if (L>maxL){ maxL=L; A0=p; B0=q }
                }
            }
            var ux=B0.x-A0.x, uy=B0.y-A0.y, L=Math.sqrt(ux*ux+uy*uy)||1; ux/=L; uy/=L
            var nx=-uy, ny=ux
            // scegli normale verso l’interno con un test PIP
            var polyXY = polygonXYOrdered()
            function pip(pt){
                var inside=false
                for (var k=0;k<polyXY.length;k++){
                    var a=polyXY[k], b=polyXY[(k+1)%polyXY.length]
                    var inter = ((a.y>pt.y)!==(b.y>pt.y)) && (pt.x < (b.x-a.x)*(pt.y-a.y)/(b.y-a.y+1e-18)+a.x)
                    if (inter) inside=!inside
                }
                return inside
            }
            var mid={x:(A0.x+B0.x)/2, y:(A0.y+B0.y)/2}
            var inPlus = pip({x:mid.x+nx*1e-3, y:mid.y+ny*1e-3})
            if (!inPlus){ nx=-nx; ny=-ny }

            function toUV(p){ var dx=p.x-A0.x, dy=p.y-A0.y; return {u:dx*ux+dy*uy, v:dx*nx+dy*ny} }
            function fromUV(uu,vv){ return {x:A0.x+uu*ux+vv*nx, y:A0.y+uu*uy+vv*ny} }

            return {A0:A0, ux:ux, uy:uy, nx:nx, ny:ny, toUV:toUV, fromUV:fromUV}
        }



        onPaint: {
            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height)
            ctx.font = labelFontPx + "px sans-serif"

            var canText = (typeof ctx.fillText === "function");
            function drawText(s, x, y) { if (canText) ctx.fillText(String(s), x, y) }

            function proj(x,y,z,label){ return projectXZ({x:x,y:y,z:z,label:label}) }

            // dimensioni piani
            var ex=100, ey=100, ez=100
            if (enuRot.length>0){
                var minx=1e9,maxx=-1e9,miny=1e9,maxy=-1e9,minz=1e9,maxz=-1e9
                for (var i=0;i<enuRot.length;i++){ var p=enuRot[i]
                    minx=Math.min(minx,p.x); maxx=Math.max(maxx,p.x)
                    miny=Math.min(miny,p.y); maxy=Math.max(maxy,p.y)
                    minz=Math.min(minz,p.z); maxz=Math.max(maxz,p.z)
                }
                ex=Math.max(50, maxx-minx); ey=Math.max(50, maxy-miny); ez=Math.max(50, maxz-minz)
            }

            // --------- piani di riferimento ---------
            ctx.globalAlpha=1.0; ctx.fillStyle = gridXZ
            var xyA = toScreen( proj(0,0,0,"") )
            var xyB = toScreen( proj(ex,0,0,"") )
            var xyC = toScreen( proj(ex,ey,0,"") )
            var xyD = toScreen( proj(0,ey,0,"") )
            ctx.beginPath(); ctx.moveTo(xyA.x,xyA.y); ctx.lineTo(xyB.x,xyB.y); ctx.lineTo(xyC.x,xyC.y); ctx.lineTo(xyD.x,xyD.y); ctx.closePath(); ctx.fill()

            ctx.fillStyle = gridYZ
            var yzA = toScreen( proj(0,0,0,"") )
            var yzB = toScreen( proj(0,ey,0,"") )
            var yzC = toScreen( proj(0,ey,ez,"") )
            var yzD = toScreen( proj(0,0,ez,"") )
            ctx.beginPath(); ctx.moveTo(yzA.x,yzA.y); ctx.lineTo(yzB.x,yzB.y); ctx.lineTo(yzC.x,yzC.y); ctx.lineTo(yzD.x,yzD.y); ctx.closePath(); ctx.fill()

            // --------- Assi con tacche ---------
            ctx.strokeStyle = axesColor; ctx.fillStyle = axesColor; ctx.lineWidth=2

            var O  = toScreen( proj(0,0,0,"") )
            var X1 = toScreen( proj(1,0,0,"") ), Z1 = toScreen( proj(0,0,1,"") )
            var ux = X1.x - O.x, uy = X1.y - O.y
            var zx = Z1.x - O.x, zy = Z1.y - O.y
            var lenX = Math.sqrt(ux*ux + uy*uy) || 1
            var lenZ = Math.sqrt(zx*zx + zy*zy) || 1
            var uxx = ux/lenX, uyy = uy/lenX
            var uzx = zx/lenZ, uzy = zy/lenZ
            var pxPerMeterX = Math.max(1e-6, lenX)
            var pxPerMeterZ = Math.max(1e-6, lenZ)

            var Lx_m = ex*0.95, Lz_m = ez*0.95

            function drawArrow(from, vx, vy, L) {
                var tox = from.x + vx*L, toy = from.y + vy*L
                ctx.beginPath(); ctx.moveTo(from.x,from.y); ctx.lineTo(tox,toy); ctx.stroke()
                var ax = tox - 10*vx + 5*vy, ay = toy - 10*vy - 5*vx
                var bx = tox - 10*vx - 5*vy, by = toy - 10*vy + 5*vx
                ctx.beginPath(); ctx.moveTo(tox,toy); ctx.lineTo(ax,ay); ctx.lineTo(bx,by); ctx.closePath(); ctx.fill()
            }
            drawArrow(O, uxx, uyy, pxPerMeterX*Lx_m)  // X′
            drawArrow(O, uzx, uzy, pxPerMeterZ*Lz_m)  // Z

            var stepX = niceStep(pxPerMeterX)
            var stepZ = niceStep(pxPerMeterZ)

            // tacche X′
            ctx.lineWidth=1
            for (var tx=stepX; tx<=Lx_m+1e-6; tx+=stepX) {
                var px = O.x + uxx * (tx*pxPerMeterX)
                var py = O.y + uyy * (tx*pxPerMeterX)
                var nx = -uyy, ny = uxx
                ctx.beginPath(); ctx.moveTo(px-3*nx, py-3*ny); ctx.lineTo(px+3*nx, py+3*ny); ctx.stroke()
                drawText(root.fmtMeters(root.q(tx)) + " m", px + 6, py - 6)
            }
            // tacche Z
            for (var tz=stepZ; tz<=Lz_m+1e-6; tz+=stepZ) {
                var qx = O.x + uzx * (tz*pxPerMeterZ)
                var qy = O.y + uzy * (tz*pxPerMeterZ)
                var nnx = -uzy, nny = uzx
                ctx.beginPath(); ctx.moveTo(qx-3*nnx, qy-3*nny); ctx.lineTo(qx+3*nnx, qy+3*nny); ctx.stroke()
                drawText(root.fmtMeters(root.q(tz)) + " m", qx + 6, qy - 6)
            }
            drawText("+X′ (φ)", O.x + uxx*(pxPerMeterX*Lx_m) + 8, O.y + uyy*(pxPerMeterX*Lx_m) - 6)
            drawText("+Z",       O.x + uzx*(pxPerMeterZ*Lz_m) + 8, O.y + uzy*(pxPerMeterZ*Lz_m) - 6)

            // --------- Poligono ----------
            if (showPolygon && enuRot.length>=2) {
                var idxs = polygonIndices()
                if (idxs.length>=2) {
                    ctx.lineWidth = polygonWidth
                    ctx.strokeStyle = polygonStroke
                    ctx.fillStyle = polygonFill
                    ctx.beginPath()
                    var p0s = toScreen( projectXZ( enuRot[idxs[0]] ) )
                    ctx.moveTo(p0s.x, p0s.y)
                    for (var ii=1; ii<idxs.length; ii++) {
                        var pis = toScreen( projectXZ( enuRot[idxs[ii]] ) )
                        ctx.lineTo(pis.x, pis.y)
                    }
                    ctx.closePath()
                    ctx.fill()
                    ctx.stroke()
                }
            }

            // --------- Mesh (tri / grid / snake / ribbons / ortho) ----------
            if (showMesh) {
                if (meshMode === "tri") {
                    var polyT = polygon2DOrdered()
                    if (polyT.length>=3){
                        if (polyT.length>3) {
                            var last=polyT[polyT.length-1], first=polyT[0]
                            if (Math.abs(last.x-first.x)<1e-9 && Math.abs(last.y-first.y)<1e-9) polyT.pop()
                        }
                        var tris = earClipTriangulate(polyT)
                        ctx.save(); ctx.strokeStyle = meshTriColor; ctx.lineWidth = meshWidth
                        for (var t=0; t<tris.length; ++t){
                            var A=toScreen(tris[t][0]), B=toScreen(tris[t][1]), C=toScreen(tris[t][2])
                            ctx.beginPath(); ctx.moveTo(A.x,A.y); ctx.lineTo(B.x,B.y); ctx.lineTo(C.x,C.y); ctx.closePath(); ctx.stroke()
                        }
                        ctx.restore()

                        // === PUNTI DEL PERCORSO A QUOTE COSTANTI (serpentina dall’alto a sinistra) ===
                        var sweepPts = buildContourSweepByZ(sweepStep)   // usa property sweepStep
                        ctx.save()
                        ctx.fillStyle = "#d33"   // rosso
                        for (var r=0; r<sweepPts.length; ++r){
                            // proietta (X′,Y′,Z) → canvas
                            var scr = toScreen( projectXZ({x:sweepPts[r].x, y:sweepPts[r].y, z:sweepPts[r].z, label:""}) )
                            ctx.beginPath(); ctx.arc(scr.x, scr.y, 3.5, 0, Math.PI*2); ctx.fill()
                        }
                        ctx.restore()


                        // Calcolo LLA (salvato in scanPathLLA)
                        scanPathLLA = []
                        for (var p=0; p<sweepENU.length; ++p){
                            var u = sweepENU[p].z + zOffsetENU  // riaggiungo l’offset rimosso in rebuild()
                            var ecef = enuToECEF(sweepENU[p].x, sweepENU[p].y, u, refLLA)
                            var lla  = ecefToGeodetic(ecef.x, ecef.y, ecef.z)
                            scanPathLLA.push({lat:lla.lat, lon:lla.lon, alt:lla.alt})
                        }
                    }
                } else if (meshMode === "grid") {
                    // === Structured grid by transfinite interpolation ===
                    var chainsG = splitChains(meshStartLabel, meshEndLabel)
                    var baseG = chainsG.c1
                    var roofG = chainsG.c2
                    if (baseG.length>=2 && roofG.length>=2) {
                        var LbG = cumLen(baseG), LrG = cumLen(roofG)
                        var LbTotG = LbG[LbG.length-1], LrTotG = LrG[LrG.length-1]
                        var stepU = (meshGridStepU>0?meshGridStepU: (LbTotG/10))
                        var ribs=[]
                        for (var su=0; su<=LbTotG+1e-6; su+=stepU) {
                            var pb = pointAt(baseG, LbG, su)
                            var pr = pointAt(roofG, LrG, su * (LrTotG/LbTotG))
                            ribs.push({b:pb, r:pr})
                        }
                        if (ribs.length<2 || Math.abs(LbTotG - ( (ribs.length-1)*stepU )) > 1e-3) {
                            ribs.push({ b: pointAt(baseG, LbG, LbTotG), r: pointAt(roofG, LrG, LrTotG) })
                        }
                        function dist(a,b){ var dx=a.x-b.x, dy=a.y-b.y; return Math.sqrt(dx*dx+dy*dy) }
                        var len0 = dist(ribs[0].b, ribs[0].r)
                        var stepV = (meshGridStepV>0?meshGridStepV: (len0/6))
                        var Nv = Math.max(1, Math.round(len0/stepV))
                        ctx.save(); ctx.strokeStyle = meshColor; ctx.lineWidth = meshWidth
                        for (var ri=0; ri<ribs.length; ++ri) {
                            var A = toScreen(ribs[ri].b), B = toScreen(ribs[ri].r)
                            ctx.beginPath(); ctx.moveTo(A.x,A.y); ctx.lineTo(B.x,B.y); ctx.stroke()
                        }
                        scanPathPoints = []
                        for (var j=0; j<=Nv; ++j) {
                            var tline = j / Nv
                            var ptsRow = []
                            for (var k=0; k<ribs.length; ++k) {
                                var pk = { x: ribs[k].b.x + tline*(ribs[k].r.x - ribs[k].b.x),
                                           y: ribs[k].b.y + tline*(ribs[k].r.y - ribs[k].b.y) }
                                ptsRow.push(pk)
                            }
                            ctx.beginPath()
                            var firstS = toScreen(ptsRow[0]); ctx.moveTo(firstS.x, firstS.y)
                            for (var m=1; m<ptsRow.length; ++m){ var Si = toScreen(ptsRow[m]); ctx.lineTo(Si.x,Si.y) }
                            ctx.stroke()
                            if (showScanPath) {
                                if ((scanStartFromLeft && (j%2===0)) || (!scanStartFromLeft && (j%2===1))) {
                                    for (var p=0;p<ptsRow.length;p++) scanPathPoints.push(ptsRow[p])
                                } else {
                                    for (var q=ptsRow.length-1;q>=0;q--) scanPathPoints.push(ptsRow[q])
                                }
                                if (j < Nv) {
                                    var nextT = (j+1)/Nv
                                    var ribIdx = scanStartFromLeft ? (j%2===0 ? ptsRow.length-1 : 0) : (j%2===0 ? 0 : ptsRow.length-1)
                                    var rR = ribs[ribIdx]
                                    var pNext = { x: rR.b.x + nextT*(rR.r.x - rR.b.x),
                                                y: rR.b.y + nextT*(rR.r.y - rR.b.y) }
                                    scanPathPoints.push(pNext)
                                }
                            }
                        }
                        ctx.restore()
                        if (showScanPath && scanPathPoints.length>1){
                            ctx.save(); ctx.strokeStyle = scanPathColor; ctx.lineWidth = scanPathWidth
                            ctx.beginPath(); var S0p = toScreen(scanPathPoints[0]); ctx.moveTo(S0p.x,S0p.y)
                            for (var sp=1; sp<scanPathPoints.length; ++sp){ var Spp=toScreen(scanPathPoints[sp]); ctx.lineTo(Spp.x,Spp.y) }
                            ctx.stroke(); ctx.restore()
                        }
                    }
                } else if (meshMode === "snake") {
                    // (come prima)
                    var poly = polyProjectedClosed()
                    if (poly.length>=3) {
                        var minx=1e9,maxx=-1e9,miny=1e9,maxy=-1e9
                        for (var pi=0; pi<poly.length; ++pi){ minx=Math.min(minx,poly[pi].x); maxx=Math.max(maxx,poly[pi].x); miny=Math.min(miny,poly[pi].y); maxy=Math.max(maxy,poly[pi].y) }
                        var stepX = meshStepX>0 ? meshStepX : (maxx-minx)/10
                        var ribs2 = []
                        for (var x=minx; x<=maxx+1e-9; x+=stepX) {
                            var ys = verticalCuts(x, poly)
                            if (ys.length>=2) ribs2.push({ x:x, y0:ys[0], y1:ys[ys.length-1] })
                        }
                        if (ribs2.length>=2) {
                            var dz = meshSnakeStepZ>0 ? meshSnakeStepZ : 2.0
                            var y0ref = ribs2[0].y0, y1ref = ribs2[0].y1
                            var Href  = Math.max(1e-9, y1ref - y0ref)
                            scanPathPoints = []
                            ctx.save(); ctx.strokeStyle = scanPathColor; ctx.lineWidth = scanPathWidth
                            var dirRight=true
                            for (var yref=y0ref; yref<=y1ref+1e-9; yref+=dz){
                                var tline = (yref - y0ref)/Href; if (tline<0) tline=0; if (tline>1) tline=1
                                if (dirRight) {
                                    for (var ri=0; ri<ribs2.length; ++ri) {
                                        var yA = ribs2[ri].y0 + tline*(ribs2[ri].y1 - ribs2[ri].y0)
                                        var A = {x:ribs2[ri].x, y:yA}; scanPathPoints.push(A)
                                    }
                                } else {
                                    for (var rj=ribs2.length-1; rj>=0; --rj) {
                                        var yC = ribs2[rj].y0 + tline*(ribs2[rj].y1 - ribs2[rj].y0)
                                        var C = {x:ribs2[rj].x, y:yC}; scanPathPoints.push(C)
                                    }
                                }
                                dirRight = !dirRight
                            }
                            ctx.beginPath(); var P0=toScreen(scanPathPoints[0]); ctx.moveTo(P0.x,P0.y)
                            for (var pp=1; pp<scanPathPoints.length; ++pp){ var Ps=toScreen(scanPathPoints[pp]); ctx.lineTo(Ps.x,Ps.y) }
                            ctx.stroke(); ctx.restore()
                        }
                    }
                } else if (meshMode === "ribbons") {
                    // (come prima)
                    var chains = splitChains(meshStartLabel, meshEndLabel)
                    var base  = chains.c1
                    var roof  = chains.c2
                    if (base.length>=2 && roof.length>=2) {
                        var Lb = cumLen(base), Lr = cumLen(roof)
                        var LbTot = Lb[Lb.length-1], LrTot = Lr[Lr.length-1]
                        ctx.save(); ctx.strokeStyle = meshColor; ctx.lineWidth = meshWidth; ctx.fillStyle = meshColor
                        for (var s=0; s<=LbTot+1e-6; s+=meshStep) {
                            var pb = pointAt(base, Lb, s)
                            var pr = pointAt(roof, Lr, s * (LrTot/LbTot))
                            var sb = toScreen(pb), sr = toScreen(pr)
                            ctx.beginPath(); ctx.moveTo(sb.x, sb.y); ctx.lineTo(sr.x, sr.y); ctx.stroke()
                        }
                        ctx.restore()
                    }
                } else {
                    // ortho ... (come prima)
                    var poly2 = polyProjectedClosed()
                    if (poly2.length>=3) {
                        var minx2=1e9,maxx2=-1e9,miny2=1e9,maxy2=-1e9
                        for (var pi2=0; pi2<poly2.length; ++pi2){ minx2=Math.min(minx2,poly2[pi2].x); maxx2=Math.max(maxx2,poly2[pi2].x); miny2=Math.min(miny2,poly2[pi2].y); maxy2=Math.max(maxy2,poly2[pi2].y) }
                        var stepVX = (meshStepX>0?meshStepX:meshStep)
                        var stepHZ = (meshStepZ>0?meshStepZ:meshStep)
                        ctx.save(); ctx.strokeStyle = meshColor; ctx.lineWidth = meshWidth
                        if (meshDrawVertical) {
                            for (var x=minx2+stepVX; x<=maxx2-stepVX+1e-9; x+=stepVX) {
                                var ys = verticalCuts(x, poly2)
                                for (var k=0; k+1<ys.length; k+=2) {
                                    var s1 = toScreen({x:x, y:ys[k]}), s2 = toScreen({x:x, y:ys[k+1]})
                                    ctx.beginPath(); ctx.moveTo(s1.x,s1.y); ctx.lineTo(s2.x,s2.y); ctx.stroke()
                                }
                            }
                        }
                        if (meshDrawHorizontal) {
                            for (var y2=miny2+stepHZ; y2<=maxy2-stepHZ+1e-9; y2+=stepHZ) {
                                var xs = horizontalCuts(y2, poly2)
                                for (var h=0; h+1<xs.length; h+=2) {
                                    var t1 = toScreen({x:xs[h], y:y2}), t2 = toScreen({x:xs[h+1], y:y2})
                                    ctx.beginPath(); ctx.moveTo(t1.x,t1.y); ctx.lineTo(t2.x,t2.y); ctx.stroke()
                                }
                            }
                        }
                        ctx.restore()
                    }
                }
            }

            // --------- Punti + etichette ---------
            ctx.fillStyle = pointColor
            ctx.strokeStyle = "#333333"
            ctx.lineWidth = 1
            for (var k=0;k<enuRot.length;k++){
                var p3 = enuRot[k]
                var s = toScreen( proj(p3.x,p3.y,p3.z,p3.label) )
                ctx.beginPath(); ctx.arc(s.x, s.y, 4, 0, Math.PI*2); ctx.fill()
                var txlab = s.x + 6, tylab = s.y - 6
                ctx.fillStyle = "#ffffff"; drawText(p3.label, txlab+1, tylab+1)
                ctx.fillStyle = labelColor; drawText(p3.label, txlab,   tylab)
                ctx.fillStyle = pointColor
            }
        }

        // pan/zoom
        MouseArea {
            anchors.fill: parent
            property real lastX; property real lastY
            onPressed: { lastX=mouse.x; lastY=mouse.y }
            onPositionChanged: if (mouse.buttons & Qt.LeftButton) {
                panX += mouse.x-lastX; panY += mouse.y-lastY; lastX=mouse.x; lastY=mouse.y; cv.requestPaint()
            }
            onWheel: {
                var old=zoom
                zoom = Math.max(0.25, Math.min(5.0, zoom * (wheel.angleDelta.y>0?1.1:0.9)))
                var mx=wheel.x, my=wheel.y
                panX = mx - (zoom/old)*(mx - panX)
                panY = my - (zoom/old)*(my - panY)
                cv.requestPaint()
            }
        }
    }

    // (ri)calcolo
    Component.onCompleted: rebuild()
    Connections {
        target: root
        onGpsPointsChanged: rebuild()
        onPhiDegChanged: rebuildProjectionOnly()
    }

    // === Ricostruzioni ===
    function rebuild() {
        if (!gpsPoints || gpsPoints.length===0) { enuRaw=[]; enuRot=[]; projPoints=[]; cv.requestPaint(); return }

        // 1) baseline Z=0
        var minAlt = gpsPoints[0].alt
        for (var i=1;i<gpsPoints.length;i++) minAlt = Math.min(minAlt, gpsPoints[i].alt)

        // 2) ref ENU: lon minima
        var idxLeft = 0, minLon = gpsPoints[0].lon
        for (var k=1;k<gpsPoints.length;k++) if (gpsPoints[k].lon < minLon) { minLon = gpsPoints[k].lon; idxLeft = k }
        var left = gpsPoints[idxLeft]
        refLLA = { lat: left.lat, lon: left.lon, alt: minAlt }

        // 3) ENU e Z>=0
        var tmp=[], minZ=1e9
        for (var j=0;j<gpsPoints.length;j++){
            var g=gpsPoints[j], lab=(g.label!==undefined? g.label : String(j))
            var ecef = geodeticToECEF(g.lat, g.lon, g.alt)
            var enu  = ecefToENU(ecef.x, ecef.y, ecef.z, refLLA)
            tmp.push({x:enu.x, y:enu.y, z:enu.z, label: lab})
            if (enu.z < minZ) minZ = enu.z
        }
        zOffsetENU = minZ
        for (var n=0;n<tmp.length;n++)
            tmp[n] = {x:tmp[n].x, y:tmp[n].y, z:tmp[n].z - minZ, label: tmp[n].label}
        enuRaw = tmp

        // 4) PCA su (x,y) → α
        var alpha = 0
        if (enuRaw.length >= 2) {
            var mx=0,my=0
            for (var a=0;a<enuRaw.length;a++){ mx+=enuRaw[a].x; my+=enuRaw[a].y }
            mx/=enuRaw.length; my/=enuRaw.length
            var sxx=0, syy=0, sxy=0
            for (var b=0;b<enuRaw.length;b++){
                var dx=enuRaw[b].x-mx, dy=enuRaw[b].y-my
                sxx+=dx*dx; syy+=dy*dy; sxy+=dx*dy
            }
            alpha = 0.5 * Math.atan2(2*sxy, (sxx - syy))
        }
        _alphaDeg = ((alpha*180/Math.PI) % 360 + 360) % 360

        // 5) ruoto i dati su (X′,Y′)
        var ca = Math.cos(alpha), sa = Math.sin(alpha)
        var rot=[], minXp=1e9, sumYp=0
        for (var r=0;r<enuRaw.length;r++){
            var Xp =  enuRaw[r].x*ca + enuRaw[r].y*sa
            var Yp = -enuRaw[r].x*sa + enuRaw[r].y*ca
            rot.push({x:Xp, y:Yp, z:enuRaw[r].z, label: enuRaw[r].label})
            if (Xp < minXp) minXp = Xp
            sumYp += Yp
        }
        enuRot = rot

        // 6) origine fissa in X′Y′
        x0 = (minXp===1e9 ? 0 : minXp)
        y0 = (enuRot.length ? (sumYp/enuRot.length) : 0)

        rebuildProjectionOnly()
    }

    function rebuildProjectionOnly() {
        var tmp2D=[], bminx=1e9,bmaxx=-1e9,bminy=1e9,bmaxy=-1e9
        for (var p=0;p<enuRot.length;p++){
            var p2 = projectXZ(enuRot[p])
            tmp2D.push(p2)
            bminx=Math.min(bminx,p2.x); bmaxx=Math.max(bmaxx,p2.x)
            bminy=Math.min(bminy,p2.y); bmaxy=Math.max(bmaxy,p2.y)
        }
        projPoints = tmp2D
        var mxm=0.15*(bmaxx-bminx || 1), mym=0.15*(bmaxy-bminy || 1)
        bounds2D = {minx:bminx-mxm, maxx:bmaxx+mxm, miny:bminy-mym, maxy:bmaxy+mym}
        cv.requestPaint()
    }
}
