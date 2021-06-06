#pragma once

#include "TransectStyleComplexItem.h"
#include "MissionModel.h"

Q_DECLARE_LOGGING_CATEGORY(TransectStyleFenceSupportedComplexItemLog)

class TransectStyleFenceSupportedComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    TransectStyleFenceSupportedComplexItem(PlanMasterController* masterController, bool flyView, QString settignsGroup, QObject* parent);
    
    Q_PROPERTY(QList<QVariantList>     visualAvoidances        READ visualAvoidances                               NOTIFY visualAvoidancesChanged)
    
    int  lastSequenceNumber (void) const final;
    QList<QVariantList> visualAvoidances(void) { return _visualAvoidances; }
    virtual double getYaw(void) = 0;

signals:
    void visualAvoidancesChanged    (void);

protected slots:
    void _rebuildTransects  (void);
    void _listenFences      (void);

protected:
    QList<QVariantList> _visualAvoidances;

    MissionModel _model;
};
