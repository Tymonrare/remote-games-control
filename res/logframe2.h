#ifndef LOGFRAME2_H
#define LOGFRAME2_H

#include <QFrame>

namespace Ui {
class logframe2;
}

class logframe2 : public QFrame
{
    Q_OBJECT

public:
    explicit logframe2(QWidget *parent = 0);
    ~logframe2();

private:
    Ui::logframe2 *ui;
};

#endif // LOGFRAME2_H
