package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/service"
	"smart-home/utils/errmsg"
)

func StoreDeviceData(c *gin.Context) {
	var deviceData model.DeviceData
	c.ShouldBindJSON(&deviceData)
	code := service.StoreDeviceData(deviceData)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
	})
}

func GetDeviceData(c *gin.Context) {
	devName := c.Param("devName")
	deviceData, code := service.GetDeviceData(devName)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		"data":   deviceData,
	})
}

func GetDeviceList(c *gin.Context) {
	deviceData, code := service.GetDeviceList()
	devList := make([]string, len(deviceData))
	for index, data := range deviceData {
		devList[index] = data.DevName
	}
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		//"data": gin.H{
		//	"dev_list": devList,
		//},
		"data": devList,
	})
}
