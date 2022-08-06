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

    bool run();

};


class ydTestList : QList<ydTest*>
{
public:
    ydTestList();

    QString testResultHTML;

    bool runAllTests();

};



#endif // YDTEST_H
