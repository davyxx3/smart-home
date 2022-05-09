package v2

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/utils/errmsg"
)

func GetSensorData(c *gin.Context) {
	// 进行查询
	dataGroup, total, code := model.GetAllSensorData()
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
		"data":   model.DataWrapper{dataGroup, total},
	})
}
