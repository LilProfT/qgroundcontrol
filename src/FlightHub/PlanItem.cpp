#include "PlanItem.h"

PlanItem::PlanItem(int id ,QString name,QObject *parent)
    : QObject{parent}
{
    _id = id;
    _name = name;

}

PlanItem::~PlanItem(){

}
