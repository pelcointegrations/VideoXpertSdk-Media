#include "stdafx.h"
#include "Utilities.h"

using namespace std;

namespace MediaController {
    namespace Utilities {
        string UnixTimeToRfc3339(unsigned int unixTime) {
            time_t unixTimeValue = unixTime;
            tm tmTime;
#ifndef WIN32
            gmtime_r(&unixTimeValue, &tmTime);
#else
            gmtime_s(&tmTime, &unixTimeValue);
#endif
            char date[Constants::kDateMaxSize];
            strftime(date, sizeof(date), "%Y%m%dT%H%M%S.000Z-", &tmTime);
            return string(date);
        }

        unsigned int CurrentUnixTime() {
            time_t currentTime;
            time(&currentTime);
            return static_cast<unsigned int>(currentTime);
        }

        long TzOffset() {
            tm utcTm{ 0 }, localTm{ 0 };
            time_t local = time(nullptr);
            gmtime_s(&utcTm, &local);
            localtime_s(&localTm, &local);
            localTm.tm_isdst = 0;
            local = mktime(&localTm);
            time_t utc = mktime(&utcTm);
            return static_cast<long>(difftime(local, utc));
        }
    }
}
