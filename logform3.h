#ifndef LOGFORM3_H
#define LOGFORM3_H

#include <QWidget>

namespace Ui {
class logform3;
}

class logform3 : public QWidget
{
    Q_OBJECT

public:
    explicit logform3(QWidget *parent = 0);
    ~logform3();

private:
    Ui::logform3 *ui;
};

#endif // LOGFORM3_H
