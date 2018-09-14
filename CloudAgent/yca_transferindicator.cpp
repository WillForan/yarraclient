#include <QDesktopWidget>
#include <QMenu>
#include <QMovie>

#include "yca_global.h"
#include "yca_transferindicator.h"
#include "ui_yca_transferindicator.h"


ycaTransferIndicator::ycaTransferIndicator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ycaTransferIndicator)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::Tool;
    setWindowFlags(flags);


    QPalette p = palette();
    p.setColor(QPalette::Background, QColor(0,0,0));
    this->setPalette(p);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignBottom, QSize(120,18), qApp->desktop()->availableGeometry()));

    anim=new QMovie(":/images/indicator.gif");
    ui->indicatorLabel->setMovie(anim);
    anim->stop();

    mainDialog=0;

    ui->indicatorLabel->setToolTip("<strong>Yarra Cloud Agent</strong> v "+QString(YCA_VERSION)+"<br><br>Transfer is active...<br><br>Double-click to learn more");
    this->setAttribute(Qt::WA_AlwaysShowToolTips, true);
}


ycaTransferIndicator::~ycaTransferIndicator()
{
    delete anim;
    delete ui;
}


void ycaTransferIndicator::showIndicator()
{
    anim->start();
    this->show();
}


void ycaTransferIndicator::hideIndicator()
{
    this->hide();
    anim->stop();
}


void ycaTransferIndicator::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (mainDialog)
    {
        mainDialog->show();
    }
}


void ycaTransferIndicator::setMainDialog(QDialog* parent)
{
    mainDialog=parent;
}

