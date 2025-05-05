#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariant>
#include <QColor>
#include <QDateTime>

class KmlPolygonObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QVariantList coordinates READ coordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int hmin READ hmin WRITE setHmin NOTIFY hminChanged)
    Q_PROPERTY(int hmax READ hmax WRITE setHmax NOTIFY hmaxChanged)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QDateTime activationDate READ activationDate WRITE setActivationDate NOTIFY activationDateChanged)
    Q_PROPERTY(QDateTime deactivationDate READ deactivationDate WRITE setDeactivationDate NOTIFY deactivationDateChanged)

public:
    KmlPolygonObject(const QString& name,
                     const QList<QGeoCoordinate>& coords,
                     const QColor& color = Qt::red,
                     int hmin = 0,
                     int hmax = 9999,
                     const QString& id = "",
                     const QString& description = "",
                     const QDateTime& activationDate = QDateTime(),
                     const QDateTime& deactivationDate = QDateTime(),
                     QObject* parent = nullptr);

    QString name() const { return _name; }
    void setName(const QString& name);

    QVariantList coordinates() const;

    QColor color() const { return _color; }
    void setColor(const QColor& color);

    int hmin() const { return _hmin; }
    void setHmin(int hmin);

    int hmax() const { return _hmax; }
    void setHmax(int hmax);

    QString id() const { return _id; }
    void setId(const QString& id);

    QString description() const { return _description; }
    void setDescription(const QString& description);

    QDateTime activationDate() const { return _activationDate; }
    void setActivationDate(const QDateTime& date);

    QDateTime deactivationDate() const { return _deactivationDate; }
    void setDeactivationDate(const QDateTime& date);

    bool contains(const QGeoCoordinate& coordinate) const;

signals:
    void nameChanged();
    void coordinatesChanged();
    void colorChanged();
    void hminChanged();
    void hmaxChanged();
    void idChanged();
    void descriptionChanged();
    void activationDateChanged();
    void deactivationDateChanged();

private:
    QString _name;
    QList<QGeoCoordinate> _coordinates;

    QColor _color;
    int _hmin;
    int _hmax;
    QString _id;
    QString _description;
    QDateTime _activationDate;
    QDateTime _deactivationDate;
};
