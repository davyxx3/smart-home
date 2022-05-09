package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/utils/errmsg"
)

func StoreDeviceData(c *gin.Context) {
	var deviceData model.DeviceData
	c.ShouldBindJSON(&deviceData)
	code := model.StoreDeviceData(deviceData)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
	})
}

func GetDeviceData(c *gin.Context) {
	devName := c.Param("devName")
	deviceData, code := model.GetDeviceData(devName)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		"data":   deviceData,
	})
}
