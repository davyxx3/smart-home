package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/service"
	"smart-home/utils/errmsg"
)

func StoreRawData(c *gin.Context) {
	var rawData model.RawData
	c.ShouldBindJSON(&rawData)
	code := service.StoreRawData(rawData)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
	})
}
