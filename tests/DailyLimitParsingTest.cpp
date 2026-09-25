#include <cassert>

#include "../CpeProtocol.cpp"

int main() {
    const std::wstring fixture = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<response>\n<DayDataLimit>0MB</DayDataLimit>\n<SetDayData>0</SetDayData>\n<dailytrafficmaxlimit>0</dailytrafficmaxlimit>\n<dailyturnoffdataenable>0</dailyturnoffdataenable>\n<dailyturnoffdataswitch>0</dailyturnoffdataswitch>\n<dailyturnoffdataflag>0</dailyturnoffdataflag>\n</response>";
    CpeDailyLimitData data;
    assert(ParseDailyLimit(fixture, data));
    assert(data.dayDataLimit == L"0MB");
    assert(data.dailytrafficmaxlimit == L"0");
    assert(data.dailyturnoffdataenable == L"0");
    assert(data.dailyturnoffdataswitch == L"0");
    // 空 body 必须失败而不是崩溃
    CpeDailyLimitData bad;
    assert(!ParseDailyLimit(L"", bad));
    return 0;
}
