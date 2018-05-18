#include "logframe2.h"
#include "ui_logframe2.h"

logframe2::logframe2(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::logframe2)
{
    ui->setupUi(this);
}

logframe2::~logframe2()
{
    delete ui;
}
