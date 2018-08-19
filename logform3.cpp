#include "logform3.h"
#include "ui_logform3.h"

logform3::logform3(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::logform3)
{
    ui->setupUi(this);
}

logform3::~logform3()
{
    delete ui;
}
