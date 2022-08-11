#ifndef YDTEST_H
#define YDTEST_H

#include <QtCore>

class ydTest
{
public:
    ydTest();

    virtual QString getName();
    virtual QString getDescription();

    virtual QString getIssues();
    virtual QString getResults();

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
    bool isTerminating;

    bool runTests();
    bool cancelTests();

    int getPercentage();

    ydTestList testList;
    ysTestThread testThread;

    QString results;
    QString issues;

};



#endif // YDTEST_H

