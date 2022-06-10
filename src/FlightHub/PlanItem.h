#ifndef PLANITEM_H
#define PLANITEM_H

#include <QObject>

class PlanItem : public QObject
{
    Q_OBJECT
public:
    PlanItem(int id, QString name,QObject *parent = nullptr);
    ~PlanItem();

    Q_PROPERTY(int id READ id CONSTANT);
    Q_PROPERTY(QString name READ name CONSTANT);

    int id(void) const {return _id;}
    QString name(void) const {return _name;}
private:
    int _id;
    QString _name;

};




#endif // PLANITEM_H
