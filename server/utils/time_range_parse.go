package utils

import (
	"strconv"
	strings "strings"
	"time"
)

func TimeRangeParse(timeRangeStr string) (time.Time, time.Time) {
	timeRange := strings.SplitN(timeRangeStr, ",", 2)
	begin, _ := strconv.ParseInt(timeRange[0], 10, 64)
	end, _ := strconv.ParseInt(timeRange[1], 0, 64)
	return time.Unix(begin, 0), time.Unix(end, 0)
}
