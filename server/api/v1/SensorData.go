package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/utils"
	"smart-home/utils/errmsg"
	"strconv"
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
