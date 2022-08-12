#ifndef YD_GLOBAL_H
#define YD_GLOBAL_H

#define YD_VERSION    "0.1a"
#define YD_ICON QIcon(":/images/yd_256.png")

#define YD_COLOR_ERROR QString("#E5554F")
#define YD_COLOR_WARNING QString("#E0A526")
#define YD_COLOR_SUCCESS QString("#40C1AC")
#define YD_COLOR_INFO QString("#489FDF")

#define YD_TEXT_ERROR(A) QString("<span style=\"color: #E5554F;\"><strong>")+QString(A)+QString("</strong></span>")
#define YD_TEXT_WARNING(A) QString("<span style=\"color: #E0A526;\"><strong>")+QString(A)+QString("</strong></span>")
#define YD_TEXT_SUCCESS(A) QString("<span style=\"color: #40C1AC;\"><strong>")+QString(A)+QString("</strong></span>")

#define YD_BADGE_ERROR(A) QString("<span style=\"background-color: #E5554F; color: #FFF; width: 120px; \">&nbsp;ERROR&nbsp;</span>&nbsp;")+QString(A)
#define YD_BADGE_WARNING(A) QString("<span style=\"background-color: #E0A526; color: #FFF; \">&nbsp;WARNING&nbsp;</span>&nbsp;")+QString(A)
#define YD_BADGE_SUCCESS(A) QString("<span style=\"background-color: #40C1AC; color: #FFF; \">&nbsp;OK&nbsp;</span>&nbsp;")+QString(A)
#define YD_BADGE_INFO(A) QString("<span style=\"background-color: #489FDF; color: #FFF; \">&nbsp;INFO&nbsp;</span>&nbsp;")+QString(A)

#endif // YD_GLOBAL_H
