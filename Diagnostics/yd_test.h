#ifndef YDTEST_H
#define YDTEST_H

#include <QtCore>

class ydTest
{
public:
    ydTest();

    QString getName();
    QString getDescription();

    QString getIssuesHTML();
    QString getResultsHTML();
    QString getResultsText();

    virtual bool run();

};


class ydTestList : public QList<ydTest*>
{
};


class ydTestRunner;

class ysTestThread : public QThread
{
    Q_OBJECT

public:
    ysTestThread(ydTestRunner* myParent);
    int currentIndex;

protected:
    virtual void run();

private:
    ydTestRunner* runner;
};


class ydTestRunner
{
public:
    ydTestRunner();

    bool isActive;
    bool runTests();
    bool cancelTests();

    int getPercentage();

    ydTestList testList;
    ysTestThread testThread;

    QString testResultHTML;

};



#endif // YDTEST_H

