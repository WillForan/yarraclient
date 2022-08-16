#ifndef YD_GLOBAL_H
#define YD_GLOBAL_H

// Include global definitions from the RDS client
#include "../Client/rds_global.h"

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

#define YD_CRITICAL 1
#define YD_WARNING 2
#define YD_INFO 3

#define YD_ADDISSUE(A,B) issues += "<p style=\"margin-bottom: 4px; margin-top: 4px; \">&bull;&nbsp; "; \
                         if (B==YD_CRITICAL) { issues += "<span style=\"background-color: #E5554F; color: #FFF; width: 120px; \">&nbsp;CRITICAL&nbsp;</span> &nbsp;"; } \
                         if (B==YD_WARNING) { issues += "<span style=\"background-color: #E0A526; color: #FFF; width: 120px; \">&nbsp;WARNING&nbsp;</span> &nbsp;"; } \
                         if (B==YD_INFO) { issues += "<span style=\"background-color: #489FDF; color: #FFF; width: 120px; \">&nbsp;INFO&nbsp;</span> &nbsp;"; } \
                         issues += QString(A)+" <span style=\"color: #7f7f7f; \">("+this->getName()+")</span></p>";

#define YD_ADDRESULT(A) results +=  QString(A);
#define YD_ADDRESULT_LINE(A) results +=  "<br />" + QString(A);
#define YD_ADDRESULT_COLORLINE(A,B) results +=  "<br />"; \
                                                if (B==YD_CRITICAL) { results += "<span style=\"color: #E5554F; \">Critical:</span> " + QString(A); } \
                                                if (B==YD_WARNING) { results += "<span style=\"color: #E0A526; \">Warning:</span> " + QString(A); } \
                                                if (B==YD_INFO) { results += "<span style=\"color: #489FDF; \">Info:</span> " + QString(A); }

#define YD_RESULT_STARTSECTION results += "<p>";
#define YD_RESULT_ENDSECTION results += "</p>";

#define YD_KEY_NO_ORT_AVAILABLE "no_ort_available"
#define YD_KEY_NO_RDS_AVAILABLE "no_rds_available"
#define YD_KEY_NO_SYNGO_AVAILABLE "no_syngo_available"


#endif // YD_GLOBAL_H

