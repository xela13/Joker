#ifndef TIMECODEEDIT_H
#define TIMECODEEDIT_H

#include <QWidget>
#include <QDebug>
#include <QLineEdit>

#include "PhTools/PhTimeCode.h"




class TimecodeEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit TimecodeEdit(QWidget *parent = 0);
    ~TimecodeEdit();

    void setTCType(PhTimeCodeType tcType);

public slots:

    void isTimeCode();


signals:

    void frameChanged(PhFrame frame);

private:

    PhFrame _frame;
    PhTimeCodeType _tcType;



};

#endif // TIMECODEEDIT_H
