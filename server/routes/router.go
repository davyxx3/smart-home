package routes

import (
	"github.com/gin-gonic/gin"
	v1 "smart-home/api/v1"
	"smart-home/middleware"
	"smart-home/sql"
)

func Init() *gin.Engine {
	gin.SetMode(sql.Settings.AppMode)
	r := gin.Default()
	// 处理跨域请求
	r.Use(middleware.Cors())
	// 绑定V1路由
	rV1 := r.Group("api/v1")
	{
		rV1.POST("sensorData", v1.StoreSensorData)
		rV1.GET("sensorData/:devName", v1.GetSensorData)
		rV1.GET("sensorData/array/:devName", v1.GetSensorDataWithLimitInArray)

		rV1.GET("deviceData/:devName", v1.GetDeviceData)
		rV1.POST("deviceData", v1.StoreDeviceData)
		rV1.GET("devList", v1.GetDeviceList)

		rV1.POST("rawData", v1.StoreRawData)
	}

	return r
}
