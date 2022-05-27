package v1

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"smart-home/model"
	"smart-home/service"
	"smart-home/utils/errmsg"
)

// StoreRawData 存储微控制器发送的原始数据
// @Summary 升级版帖子列表接口
// @Description 可按社区按时间或分数排序查询帖子列表接口
// @Tags 帖子相关接口
// @Accept application/json
// @Produce application/json
// @Success 200
func StoreRawData(c *gin.Context) {
	var rawData model.RawData
	c.ShouldBindJSON(&rawData)
	code := service.StoreRawData(rawData)
	c.JSON(http.StatusOK, gin.H{
		"status": code,
		"msg":    errmsg.GetErrMsg(code),
	})
}
