#ifndef YDTEST_H
#define YDTEST_H

#include <QtCore>

typedef QMap<QString, QString> ydBroker;

class ydTest
{
public:
    ydTest();

    virtual QString getName();
    virtual QString getDescription();

    virtual QString getIssues();
    virtual QString getResults();

    virtual bool run();

    ydBroker* broker;
    void setBroker(ydBroker* brokerInstance);
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

    ydBroker broker;
};



#endif // YDTEST_H

