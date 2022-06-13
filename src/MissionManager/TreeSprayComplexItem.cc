#include "TreeSprayComplexItem.h"
#include "PlanMasterController.h"
#include "MissionSettingsItem.h"

const QString TreeSprayComplexItem::name(tr("Tưới trái cây"));

const char* TreeSprayComplexItem::jsonComplexItemTypeValue =   "tree";

const char* TreeSprayComplexItem::settingsGroup = "Tree";
const char* TreeSprayComplexItem::flowRateName  = "FlowRate";
const char* TreeSprayComplexItem::volumeName    = "Volume";

TreeSprayComplexItem::TreeSprayComplexItem(PlanMasterController* masterController)
    : ComplexMissionItem (masterController, false)
    , _metaDataMap       (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Tree.SettingsGroup.json"), this))
    , _flowRateFact      (settingsGroup, _metaDataMap[flowRateName])
    , _volumeFact        (settingsGroup, _metaDataMap[volumeName])
{
    _editorQml = "qrc:/qml/TreeItemEditor.qml";
    _isIncomplete = false;

    connect(&_flowRateFact, &SettingsFact::valueChanged, this, &TreeSprayComplexItem::sprayTimeChanged);
    connect(&_volumeFact,   &SettingsFact::valueChanged, this, &TreeSprayComplexItem::sprayTimeChanged);

    resetDefault();

    connect(&_flowRateFact, &SettingsFact::valueChanged, this, &TreeSprayComplexItem::loseDefault);
    connect(&_volumeFact,   &SettingsFact::valueChanged, this, &TreeSprayComplexItem::loseDefault);
}

void TreeSprayComplexItem::resetDefault(void)
{
    auto visualItems = _masterController->missionController()->visualItems();
    for (int i=0; i<visualItems->count(); i++) {
        MissionSettingsItem* missionSettings = qobject_cast<MissionSettingsItem*>(visualItems->get(i));
        if (missionSettings) {
            blockLoseDefault = true;
            _flowRateFact.setRawValue(QVariant::fromValue(missionSettings->flowRate()->rawValue()));
            _volumeFact.setRawValue(QVariant::fromValue(missionSettings->volume()->rawValue()));
            blockLoseDefault = false;
            setIsDefault(true);
        }
    }
}

void TreeSprayComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    MissionItem* gotPositionItem = new MissionItem(
                0,   // set it later
                MAV_CMD_NAV_WAYPOINT,
                _mavFrame,
                0.0, // hold time
                0.0,                                         // No acceptance radius specified
                0.0,                                         // Pass through waypoint
                std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                _coordinate.latitude(),
                _coordinate.longitude(),
                _coordinate.altitude(),
                true,                                        // autoContinue
                false,                                       // isCurrentItem
                missionItemParent
    );
    MissionItem* enableItem = new MissionItem(0,   // set it later
                                        216, // MAV_CMD_DO_SPRAYER,
                                        _mavFrame,
                                        1.0, // enable
                                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                        true,                                        // autoContinue
                                        false,                                       // isCurrentItem
                                        missionItemParent);
    MissionItem* holdPositionItem = new MissionItem(
                0,   // set it later
                MAV_CMD_NAV_WAYPOINT,
                _mavFrame,
                _volumeFact.rawValue().toDouble() / _flowRateFact.rawValue().toDouble() * 60, // hold time
                0.0,                                         // No acceptance radius specified
                0.0,                                         // Pass through waypoint
                std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                _coordinate.latitude(),
                _coordinate.longitude(),
                _coordinate.altitude(),
                true,                                        // autoContinue
                false,                                       // isCurrentItem
                missionItemParent
    );
    MissionItem* disableItem = new MissionItem(0,   // set it later
                                        216, // MAV_CMD_DO_SPRAYER,
                                        _mavFrame,
                                        0.0, // disable
                                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                        true,                                        // autoContinue
                                        false,                                       // isCurrentItem
                                        missionItemParent);

    int max = 1750;
    int min  = 1050;
    double ratio = (max - min) / 100;
    int pwm = min + (int) (30 * ratio);
    MissionItem* setServoItem = new MissionItem(0,   // set it later
                                                MAV_CMD_DO_SET_SERVO, // MAV_CMD_DO_SET_SERVO,
                                                _mavFrame,
                                                9, // servo 9
                                                pwm, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                                true,                                        // autoContinue
                                                false,                                       // isCurrentItem
                                                missionItemParent);

    items.append(setServoItem);
    items.append(gotPositionItem);
    items.append(enableItem);
    items.append(holdPositionItem);
    items.append(disableItem);
    // seqNum+=4;
};

void TreeSprayComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;
    saveObject[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
    saveObject[flowRateName] = _flowRateFact.rawValue().toDouble();
    saveObject[volumeName] = _volumeFact.rawValue().toDouble();
    missionItems << saveObject;
};

bool TreeSprayComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    _flowRateFact.setRawValue(QVariant::fromValue(complexObject[flowRateName]));
    _volumeFact.setRawValue(QVariant::fromValue(complexObject[volumeName]));
    return true;
};
