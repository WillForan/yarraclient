#ifndef YCT_COMMON_H
#define YCT_COMMON_H

#include <QtCore>


class yctAWSCommon
{
public:

    static const int regionCount=16;

    enum Regions
    {
        us_east_1 = 0,
        us_east_2,
        us_west_1,
        us_west_2,
        ca_central_1,
        eu_central_1,
        eu_west_1,
        eu_west_2,
        eu_west_3,
        ap_northeast_1,
        ap_northeast_2,
        ap_northeast_3,
        ap_southeast_1,
        ap_southeast_2,
        ap_south_1,
        sa_east_1
    };

    static QString getRegionID(Regions region);
    static QString getRegionName(Regions region);

};


inline QString yctAWSCommon::getRegionID(Regions region)
{
    switch (region)
    {
    default:
    case us_east_1:
        return "us-east-1";
        break;
    case us_east_2:
        return "us-east-2";
        break;
    case us_west_1:
        return "us-west-1";
        break;
    case us_west_2:
        return "us-west-2";
        break;
    case ca_central_1:
        return "ca-central-1";
        break;
    case eu_central_1:
        return "eu-central-1";
        break;
    case eu_west_1:
        return "eu-west-1";
        break;
    case eu_west_2:
        return "eu-west-2";
        break;
    case eu_west_3:
        return "eu-west-3";
        break;
    case ap_northeast_1:
        return "ap-northeast-1";
        break;
    case ap_northeast_2:
        return "ap-northeast-2";
        break;
    case ap_northeast_3:
        return "ap-northeast-3";
        break;
    case ap_southeast_1:
        return "ap-southeast-1";
        break;
    case ap_southeast_2:
        return "ap-southeast-2";
        break;
    case ap_south_1:
        return "ap-south-1";
        break;
    case sa_east_1:
        return "sa-east-1";
        break;
    }

    return "";
}


inline QString yctAWSCommon::getRegionName(Regions region)
{
    switch (region)
    {
    default:
    case us_east_1:
        return "US East (N. Virginia)";
        break;
    case us_east_2:
        return "US East (Ohio)";
        break;
    case us_west_1:
        return "US West (N. California)";
        break;
    case us_west_2:
        return "US West (Oregon)";
        break;
    case ca_central_1:
        return "Canada (Central)";
        break;
    case eu_central_1:
        return "EU (Frankfurt)";
        break;
    case eu_west_1:
        return "EU (Ireland)";
        break;
    case eu_west_2:
        return "EU (London)";
        break;
    case eu_west_3:
        return "EU (Paris)";
        break;
    case ap_northeast_1:
        return "Asia Pacific (Tokyo)";
        break;
    case ap_northeast_2:
        return "Asia Pacific (Seoul)";
        break;
    case ap_northeast_3:
        return "Asia Pacific (Osaka-Local)";
        break;
    case ap_southeast_1:
        return "Asia Pacific (Singapore)";
        break;
    case ap_southeast_2:
        return "Asia Pacific (Sydney)";
        break;
    case ap_south_1:
        return "Asia Pacific (Mumbai)";
        break;
    case sa_east_1:
        return "South America (São Paulo)";
        break;
    }

    return "";
}


#endif // YCT_COMMON_H

