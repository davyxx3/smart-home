package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/utils"
	"smart-home/utils/errmsg"
	"strconv"
	"time"
)

func StoreSensorData(c *gin.Context) {
	var sensorData model.SensorData
	c.ShouldBindJSON(&sensorData)
	code := model.StoreSensorData(sensorData)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
	})
}

func GetSensorData(c *gin.Context) {
	// 解析分页参数
	pageNum, _ := strconv.Atoi(c.DefaultQuery("page", "1"))
	pageSize, _ := strconv.Atoi(c.DefaultQuery("perPage", "5"))
	// 解析设备名
	devName := c.Param("devName")
	// 解析时间范围
	begin, end := utils.TimeRangeParse(c.DefaultQuery("timeRange", "1648569600%2C1649174400"))
	// 进行查询
	dataGroup, total, code := model.GetSensorData(devName, begin, end, pageNum, pageSize)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		"data":   model.DataWrapper{dataGroup, total},
	})
}

func GetSensorDataWithLimitInArray(c *gin.Context) {
	devName := c.Param("devName")
	limit, _ := strconv.Atoi(c.DefaultQuery("limit", "6"))
	dataGroup, code := model.GetSensorDataDescendWithLimit(devName, limit)
	temGroup := make([]float64, limit)
	timeGroup := make([]string, limit)
	humGroup := make([]float64, limit)
	coGroup := make([]float64, limit)
	pmGroup := make([]float64, limit)
	for i := limit - 1; i >= 0; i-- {
		temGroup[limit-1-i] = dataGroup[i].Tem
		timeGroup[limit-1-i] = time.Time(dataGroup[i].Time).Format("15:04")
		humGroup[limit-1-i] = dataGroup[i].Hum
		coGroup[limit-1-i] = dataGroup[i].CoCon
		pmGroup[limit-1-i] = dataGroup[i].PmCon
	}
	//for index, data := range dataGroup {
	//	temGroup[index] = data.Tem
	//	timeGroup[index] = time.Time(data.Time).Format("15:04")
	//	humGroup[index] = data.Hum
	//	coGroup[index] = data.CoCon
	//	pmGroup[index] = data.PmCon
	//}
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		"data": gin.H{
			"time":        timeGroup,
			"temperature": temGroup,
			"humidity":    humGroup,
			"CO2":         coGroup,
			"PM":          pmGroup,
		},
	})
}
