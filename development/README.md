# QGC development environment

## Purpose

This development environment uses [Dev Containers](https://containers.dev/) to provide consistent compilers, libraries, dependencies, and development tools across contributors.

The [devcontainer configuration](../.devcontainer/devcontainer.json) provides a shared environment for building and running QGC. Developers can edit the project in VS Code and display the running application in a separate window on the Windows desktop.

The application can communicate with either a simulated vehicle or a physical Pixhawk.

QGC's build and runtime environment is **containerized**. ArduPilot SITL and the Windows desktop tools are configured separately in WSL and Windows.

> This document describes the initial setup and daily development workflow in the **Windows-WSL** environment. Instructions for **macOS** are planned.

## Windows-WSL

### Architecture and Details

The architecture combines several tools, each with a specific responsibility:

| Component | Responsibility |
| --------- | -------------- |
| Windows | Hosts the desktop and the physical USB connection |
| Ubuntu WSL | Builds and runs ArduPilot SITL and its MAVProxy instance |
| Docker Desktop | Runs the QGC Linux container |
| VS Code | Provides the editor and integration with [Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers) |
| QGC devcontainer | Provides QGC's development tools, build dependencies, and runtime environment |
| VcXsrv | Displays Linux application windows on the Windows desktop |
| Windows MAVProxy | Bridges a physical Pixhawk's serial connection to QGC |

Display connections and vehicle connections follow separate paths:

- **Display communication:**

  ```text
  QGC / SITL graphical windows -> VcXsrv -> Windows desktop
  ```

- **Vehicle communication:**

  ```text
  SITL -> MAVProxy in WSL -> TCP -> QGC inside Docker
  Pixhawk USB -> MAVProxy in Windows -> TCP -> QGC inside Docker
  ```

The verified vehicle connection uses TCP. The container connects to MAVProxy through `host.docker.internal` and the configured TCP port. Windows MAVProxy opens the Pixhawk's COM port; the current devcontainer does not expose the USB serial device directly.

The devcontainer displays QGC through VcXsrv using `DISPLAY=host.docker.internal:0.0`. WSL applications connect to the same display server through the Windows host address.

The source checkout is mounted into the container at `/workspaces/qgroundcontrol`. Docker named volumes retain the following data across container recreation:

| Container directory | Persistent data |
| ------------------- | --------------- |
| `/workspace-build` | QGC build outputs and the compilation database |
| `/ccache` | Compiler cache |
| `/home/qgc` | QGC settings, application data, and user files |

### Prerequisites

#### Windows host

- **Windows 11 on an x86-64 machine**, with [WSL 2 and an Ubuntu distribution](https://learn.microsoft.com/en-us/windows/wsl/install) installed. The current container configuration targets `linux/amd64`.
- **[Docker Desktop](https://docs.docker.com/desktop/features/wsl/)** configured to use the WSL 2 backend and Linux containers.
- **[Visual Studio Code](https://code.visualstudio.com/docs/devcontainers/containers)** installed on Windows, with the following extensions:
  - [WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl)
  - [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- **[VcXsrv](https://sourceforge.net/projects/vcxsrv/)** installed on Windows. The documented workflow uses display `:0`, multiwindow mode, and TCP access to the X server on port `6000`. VcXsrv must be running before graphical applications are launched.

Windows 11 includes WSLg, but this configuration uses VcXsrv for graphical display. The workflow was verified with VcXsrv after a WSLg compositor failure was diagnosed on the development machine.

#### Ubuntu WSL

- **Git** installed in Ubuntu for cloning the repositories and initializing their submodules.
- The QGC checkout stored in the WSL filesystem, for example `~/dev/qgroundcontrol`, and opened in VS Code from a WSL session.
- Access to Docker Desktop from the Ubuntu terminal through the enabled WSL integration.
- The paths `/mnt/wslg` and `/mnt/wslg/.X11-unix` present in WSL. The current devcontainer configuration still references these paths as bind mounts, including when VcXsrv is used for display.

The workflow was verified on Ubuntu 26.04.1 LTS, x86-64. QGC uses a separate Ubuntu 20.04 container image. Its compiler, Qt 5.15.2 application libraries, Qt 6.8.0 QML tooling, clangd, and debugging tools are installed by the Dockerfile.

#### ArduPilot SITL

For simulation, prepare the following in **Ubuntu WSL**:

- An ArduPilot checkout with its Git submodules initialized.
- Build dependencies and MAVProxy installed using the [official ArduPilot Linux/Ubuntu setup procedure](https://ardupilot.org/dev/docs/building-setup-linux.html).
- The Python environment used by that installation activated when building or launching SITL.
- WSL display settings configured to reach VcXsrv on Windows.

The installation and display configuration are described in the **SITL setup** section below. The [official ArduPilot WSL guide](https://ardupilot.org/dev/docs/building-setup-windows10_new.html) also provides background on the Windows integration.

#### Physical Pixhawk

For a physical vehicle, prepare the following in **Windows**:

- A Pixhawk running MAVLink-capable firmware and a USB cable that supports data transfer.
- A recognized serial port under **Device Manager -> Ports (COM & LPT)**. Record its COM port number for the connection command.
- **[Windows MAVProxy](https://ardupilot.org/mavproxy/docs/getting_started/download_and_installation.html)** installed using the standalone Windows installer. The executable must be accessible as `mavproxy.exe` or through its full installation path.

Windows MAVProxy handles the physical USB connection. The WSL MAVProxy instance is used for SITL. The corresponding connection procedures belong in the **Connections** section below.

### QGC setup

The examples use `~/dev/qgroundcontrol` as the repository location. Replace it if your checkout is elsewhere.

#### 1. Open the project from WSL

In an **Ubuntu WSL terminal**, enter the existing checkout, initialize its submodules, and open VS Code:

```bash
cd ~/dev/qgroundcontrol
git submodule update --init --recursive
code .
```

In Docker Desktop, enable Ubuntu under **Settings -> Resources -> WSL Integration**.

In VS Code, select **Dev Containers: Reopen in Container** from the Command Palette. The first launch builds the container image and installs the development tools.

#### 2. Start the display server

Start VcXsrv from **Windows PowerShell**:

```powershell
Start-Process -FilePath "$env:ProgramFiles\VcXsrv\vcxsrv.exe" -ArgumentList ":0 -multiwindow -clipboard -ac -listen tcp"
```

Start it once and reuse the running instance. Allow the required network in Windows Firewall.

The options select separate application windows, display `:0`, clipboard sharing, and TCP connections. The `-ac` option disables X11 access control; keep the firewall rule limited to your development network.

In a **devcontainer terminal**, check the connection:

```bash
printf 'DISPLAY=%s\n' "$DISPLAY"
timeout 10 xdpyinfo -display "$DISPLAY" >/dev/null && echo "Display connection works"
```

The expected display is `host.docker.internal:0.0`.

#### 3. Build and run QGC

In the **devcontainer terminal**:

```bash
qgc-build.sh
qgc-run.sh
```

QGC uses Qt **5.15.2** and qmake. The Qt 6 installation supplies QML tooling.

The build script creates a debug build and uses four parallel jobs by default. On a constrained machine, reduce this to two:

```bash
QGC_BUILD_JOBS=2 qgc-build.sh
```

After the first build, run **clangd: Restart language server** from the VS Code Command Palette if code navigation needs refreshing.

Run `qgc-build.sh` after application changes, then restart QGC. If you change the Dockerfile, container settings, or scripts copied into the image, use **Dev Containers: Rebuild and Reopen in Container**.

### SITL setup

SITL runs ArduPilot against a simulated vehicle. Install and run it in **Ubuntu WSL**, outside the QGC devcontainer.

#### 1. Install ArduPilot

Follow the [official ArduPilot Linux setup guide](https://ardupilot.org/dev/docs/building-setup-linux.html). For a new checkout, run these commands in an **Ubuntu WSL terminal**:

```bash
mkdir -p ~/dev
cd ~/dev
git clone --recurse-submodules https://github.com/ArduPilot/ardupilot.git
cd ardupilot
Tools/environment_install/install-prereqs-ubuntu.sh -y
source ~/.profile
```

If ArduPilot is already installed, use that checkout. Wait for the prerequisite installer to finish successfully before continuing. `source ~/.profile` loads its environment changes into the current terminal.

Activate the Python environment created by the installer:

```bash
source ~/venv-ardupilot/bin/activate
```

#### 2. Configure the WSL display

With VcXsrv running, enter these commands in the **Ubuntu WSL terminal**:

```bash
export DISPLAY="$(ip -4 route show default | awk '{print $3; exit}'):0.0"
export GDK_BACKEND=x11
xterm
```

A test terminal should open on the Windows desktop. Type `exit` in that window to close it and return to the original terminal.

The display command uses the Windows host address under [WSL NAT networking](https://learn.microsoft.com/en-us/windows/wsl/networking). The address can change after WSL restarts. `GDK_BACKEND=x11` selects X11 for the MAVProxy graphical windows.

To apply these settings automatically, add the two `export` lines to `~/.zshrc` for Zsh or `~/.bashrc` for Bash. Replace any previous conflicting `DISPLAY` assignment.

These settings apply to applications launched from that WSL shell. The devcontainer has its own display setting in `devcontainer.json`.

#### 3. Build ArduCopter

In the **Ubuntu WSL terminal**, with the Python environment active:

```bash
cd ~/dev/ardupilot
./waf configure --board sitl
./waf copter -j2
```

This builds `build/sitl/bin/arducopter`. The two-job limit reduces build resource usage.

#### 4. Start the first simulation

In the **Ubuntu WSL terminal**:

```bash
cd ~/dev/ardupilot/ArduCopter
../Tools/autotest/sim_vehicle.py --no-rebuild -v ArduCopter -f quad -w \
  --map --console --no-extra-ports --out=tcpin:0.0.0.0:5777
```

The [official SITL guide](https://ardupilot.org/dev/docs/setting-up-sitl-on-linux.html) recommends `-w` for the first launch. It resets saved simulator parameters to their defaults. Leave it out during normal daily use.

The launch opens the ArduCopter terminal, Map, and Console windows. MAVProxy's command prompt stays in the WSL terminal.

Wait for `Detected vehicle 1:1` and the parameter download. Keep the WSL terminal and simulator windows open.

### Daily Startup

Use this sequence after completing the initial setup.

1. Start **Docker Desktop** and **VcXsrv**. If VcXsrv is already running, reuse it. Otherwise use the PowerShell command in [QGC setup](#2-start-the-display-server).
2. Start your vehicle source. For **SITL**, run this in an **Ubuntu WSL terminal**:

   ```bash
   source ~/venv-ardupilot/bin/activate
   export DISPLAY="$(ip -4 route show default | awk '{print $3; exit}'):0.0"
   export GDK_BACKEND=x11
   cd ~/dev/ardupilot/ArduCopter
   ../Tools/autotest/sim_vehicle.py --no-rebuild -v ArduCopter -f quad \
     --map --console --no-extra-ports --out=tcpin:0.0.0.0:5777
   ```

   If the display exports are saved in your shell configuration, you can omit those two lines. Remove `--no-rebuild` when you need to rebuild ArduPilot.

   For a **physical Pixhawk**, follow the Windows MAVProxy steps in [Physical Pixhawk](#physical-pixhawk-1) below. Use one vehicle source at a time when sharing port `5777`.
3. Open the QGC checkout in VS Code from WSL and select **Dev Containers: Reopen in Container**.
4. In the **devcontainer terminal**, run:

   ```bash
   qgc-run.sh
   ```

   Run `qgc-build.sh` first if application code has changed or has not been built yet.
5. In QGC, select the saved TCP link described below and click **Connect**.

Commands such as `mode guided` belong in the MAVProxy command prompt showing `STABILIZE>` or `GUIDED>`. The separate **Console** window displays vehicle status.

For SITL, the prompt is in the WSL terminal. For a physical Pixhawk, it is in the Windows terminal running MAVProxy.

### Connections

Both workflows use a MAVProxy TCP listener and the same QGC settings. MAVProxy supports this through its [documented TCP output option](https://ardupilot.org/mavproxy/docs/getting_started/faq.html).

| Port | Purpose |
| ---- | ------- |
| TCP `6000` | VcXsrv display `:0` |
| TCP `5760` | ArduCopter SITL to MAVProxy, within WSL |
| TCP `5777` | MAVProxy to QGC, for the selected vehicle source |

QGC makes an outgoing connection to MAVProxy. No additional Docker TCP port publishing is needed for this workflow. The existing Docker UDP mapping `14560:14551/udp` is separate from the TCP connection used here.

#### SITL

The launch options have the following roles:

- `-v ArduCopter -f quad`: select the vehicle and frame.
- `--map --console`: open the graphical MAVProxy windows.
- `--no-extra-ports`: skip the launcher's additional default telemetry outputs.
- `--out=tcpin:0.0.0.0:5777`: make MAVProxy listen for a QGC TCP connection.

In QGC, open **Application Settings -> Comm Links -> Add**:

| Setting | Value |
| ------- | ----- |
| Name | `ArduPilot TCP` |
| Type | `TCP` |
| Server Address | `host.docker.internal` |
| Port | `5777` |

Save the link, select it, and click **Connect**. Wait for the simulated vehicle and its parameters to appear.

Use `host.docker.internal` from inside the container. This address was verified in our setup; connecting directly to the WSL IP failed in the tested Docker network.

#### Physical Pixhawk

This workflow uses the Pixhawk's real firmware and USB connection.

1. Stop SITL and disconnect the QGC link before reusing port `5777`.
2. Connect the Pixhawk to Windows using a USB data cable.
3. Open **Device Manager -> Ports (COM & LPT)** and identify its COM port.
4. Close any other application using that serial port.
5. Start Windows MAVProxy from **Windows PowerShell**:

   ```powershell
   mavproxy.exe --master=COM4 --out=tcpin:0.0.0.0:5777
   ```

   Replace `COM4` with the actual port. If `mavproxy.exe` is not on your command path, run the installed executable through its full path.
6. Wait for MAVProxy to detect the vehicle.
7. In QGC, connect the saved `ArduPilot TCP` link to `host.docker.internal:5777`.

The [MAVProxy USB quickstart](https://ardupilot.org/mavproxy/docs/getting_started/quickstart.html) describes serial connections. Keep Windows MAVProxy running while QGC is connected.

In the **Windows MAVProxy prompt**, check the connection:

```text
link list
link
output list
```

The master link should show the selected COM port. Run `link` again after a few seconds: the received packet count should increase. The output should show port `5777`.

Vehicle detection and parameter retrieval in QGC confirm that the hardware telemetry reaches the application.

### Verification and Troubleshooting

#### Check a simulated flight

Use this test with **SITL**, after confirming that `link list` shows `tcp:127.0.0.1:5760` and QGC is connected to the simulated vehicle.

In the **WSL MAVProxy prompt**, enter one command at a time:

```text
mode guided
arm throttle
takeoff 10
```

Wait for the arming confirmation, then issue takeoff within 15 seconds. The simulated vehicle should rise to about 10 metres above home, and QGC should show the altitude and mode changes.

Select **LAND** as the flight mode in QGC. MAVProxy should report `LAND`, followed by decreasing altitude and eventual disarming. This checks commands from QGC back to SITL.

You can also land from MAVProxy:

```text
mode land
```

If arming fails, read the `PreArm:` message and address the reported condition. See the [official Copter SITL tutorial](https://ardupilot.org/dev/docs/copter-sitl-mavproxy-tutorial.html) for more examples.

#### QGC cannot open its window

Check VcXsrv in **Windows PowerShell**:

```powershell
Get-Process vcxsrv -ErrorAction SilentlyContinue
Get-NetTCPConnection -State Listen -LocalPort 6000 -ErrorAction SilentlyContinue
```

If nothing is listening, start VcXsrv with the command in **QGC setup**.

In the **devcontainer terminal**, repeat the display check:

```bash
printf 'DISPLAY=%s\n' "$DISPLAY"
timeout 10 xdpyinfo -display "$DISPLAY" >/dev/null && echo "Display connection works"
```

If VcXsrv is listening but the connection fails, check Windows Firewall access for TCP port `6000`. A mounted X11 socket alone does not prove that a display server is reachable.

#### QGC does not detect a vehicle

Check these points in order:

1. In the MAVProxy prompt, run `link`. Confirm that the packet count increases.
2. Run `output list`. Confirm that the output uses port `5777`.
3. Check that QGC uses **TCP**, `host.docker.internal`, and port `5777`, then click **Connect**.
4. Check that another SITL or hardware MAVProxy instance is not using the same port.

For Windows MAVProxy, inspect the listener in **Windows PowerShell**:

```powershell
Get-NetTCPConnection -State Listen -LocalPort 5777 -ErrorAction SilentlyContinue
```

If that listener exists but QGC cannot connect, check Windows Firewall permission for MAVProxy on TCP port `5777`.

#### The Pixhawk COM port is missing or busy

First confirm that Windows recognizes the device under **Ports (COM & LPT)**. Check the USB data cable and the device's driver if the port is missing.

If MAVProxy cannot open an existing port, close other applications using it, including other ground stations or MAVProxy instances. Confirm that the command uses the current COM number.

#### The terminal repeatedly prints GTK warnings

A message such as this concerns the graphical window's [layout calculation](https://docs.gtk.org/gtk3/func.distribute_natural_allocation.html):

```text
Gtk-CRITICAL: gtk_distribute_natural_allocation: assertion 'extra_space >= 0' failed
```

In the inspected setup, the warning came from the MAVProxy **Console** window. Enlarge or maximize it and check whether new warnings stop. Existing messages remain in the terminal.

The warning alone does not mean SITL has stopped. Check live telemetry and command responses.

If the warning continues, temporarily unload the Console from the **MAVProxy prompt**:

```text
module unload console
```

SITL and the QGC connection can continue running. Restore the window with `module load console`.

---

## macOS

The macOS workflow has not been validated yet. The current configuration uses WSL-specific mounts and a Windows display server address. A macOS setup will require platform-specific display, mount, and hardware connection instructions.
