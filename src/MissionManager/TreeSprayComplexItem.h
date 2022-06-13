#ifndef TREESPRAYCOMPLEXITEM_H
#define TREESPRAYCOMPLEXITEM_H

#include "ComplexMissionItem.h"

class TreeSprayComplexItem : public ComplexMissionItem
{
    Q_OBJECT
public:
    TreeSprayComplexItem(PlanMasterController* masterController);

    Q_PROPERTY(Fact* flowRate               READ flowRate               CONSTANT)
    Q_PROPERTY(Fact* volume                 READ volume                 CONSTANT)
    Q_PROPERTY(double sprayTime             READ additionalTimeDelay    NOTIFY sprayTimeChanged)
    Q_PROPERTY(bool isDefault               READ isDefault              WRITE setIsDefault NOTIFY isDefaultChanged)

    Q_INVOKABLE void resetDefault (void);

    Fact* flowRate (void) { return &_flowRateFact; };
    Fact* volume   (void) { return &_volumeFact; };
    bool  isDefault (void) { return _isDefault; };
    void  setIsDefault (bool val) { _isDefault = val; emit isDefaultChanged(); };
    void  loseDefault (void) { _isDefault = false; if (!blockLoseDefault) emit isDefaultChanged(); };

    // Overrides from ComplexMissionItem
    QString patternName         (void) const final { return name; }
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString mapVisualQML        (void) const final {
        qDebug() << "TreeSprayComplexItem::mapVisualQML()";
        return QStringLiteral("SimpleItemMapVisual.qml");
    }
//    QString presetsSettingsGroup(void) override { return settingsGroup; }
//    void    savePreset          (const QString& name) override;
//    void    loadPreset          (const QString& name) override;
    int     sequenceNumber      (void) const final { return _sequenceNumber; }
    int     lastSequenceNumber  (void) const final { return sequenceNumber(); };
//    void    addKMLVisuals       (KMLPlanDomDocument& domDocument) final { /* try empty first */ };
    double  complexDistance     (void) const final { return 0; }
    double  greatestDistanceTo  (const QGeoCoordinate &other) const final { return other.distanceTo(coordinate()); };

    // Overrides from VisualMissionItem
    void save                   (QJsonArray&  missionItems) final;
    void appendMissionItems     (QList<MissionItem*>& items, QObject* missionItemParent) final;
    bool exitCoordinateSameAsEntry (void) const final { return true; };
    QGeoCoordinate coordinate() const final { return _coordinate; };

    // Overrides from VisualMissionItem
    bool                specifiesCoordinate         (void) const final { return true; };
    void                applyNewAltitude            (double newAltitude) final { _coordinate.setAltitude(newAltitude); };
    bool                dirty                       (void) const final { return _dirty; }
    bool                isSimpleItem                (void) const final { return false; }
    bool                isStandaloneCoordinate      (void) const final { return false; }
    bool                specifiesAltitudeOnly       (void) const final { return false; }
    QGeoCoordinate      exitCoordinate              (void) const final { return _coordinate; }
    double              specifiedFlightSpeed        (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalYaw          (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalPitch        (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    // void                setMissionFlightStatus      (MissionController::MissionFlightStatus_t& missionFlightStatus) final;
    ReadyForSaveState   readyForSaveState           (void) const override { return ReadyForSave; };
    QString             commandDescription          (void) const override { return tr("Tree"); }
    QString             commandName                 (void) const override { return tr("Tree"); }
    QString             abbreviation                (void) const override { return tr("C"); }
    void                setDirty                    (bool dirty) final { _dirty = dirty; };
    void                setCoordinate               (const QGeoCoordinate& coordinate) final { _coordinate = coordinate; }
    void                setSequenceNumber           (int sequenceNumber) final { _sequenceNumber = sequenceNumber; };
    double              amslEntryAlt                (void) const final { return 0; };
    double              amslExitAlt                 (void) const final { return 0; };
    double              minAMSLAltitude             (void) const final { return _coordinate.altitude(); };
    double              maxAMSLAltitude             (void) const final { return _coordinate.altitude(); };
    double              additionalTimeDelay         (void) const final { return _volumeFact.rawValue().toDouble() / _flowRateFact.rawValue().toDouble() * 60; };

    static const QString name;
    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* flowRateName;
    static const char* volumeName;

    QGeoCoordinate _coordinate;

    bool blockLoseDefault = false;

signals:
    void sprayTimeChanged();
    void isDefaultChanged();

private:
    MAV_FRAME _mavFrame = MAV_FRAME_GLOBAL;
    int _sequenceNumber = 0;
    int _sprayTime = 0;

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact _flowRateFact;
    SettingsFact _volumeFact;

    bool _isDefault = true;
};

#endif // TREESPRAYCOMPLEXITEM_H
