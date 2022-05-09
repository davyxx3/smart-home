package routes

import (
	"github.com/gin-gonic/gin"
	v1 "smart-home/api/v1"
	v2 "smart-home/api/v2"
	"smart-home/middleware"
	"smart-home/model"
)

func InitRouter() {
	gin.SetMode(model.Settings.AppMode)
	r := gin.Default()
	r.Use(middleware.Cors())
	rV1 := r.Group("api/v1")
	{
		rV1.POST("sensorData", v1.StoreSensorData)
		rV1.GET("sensorData/:devName", v1.GetSensorData)
		rV1.GET("sensorData/array/:devName", v1.GetSensorDataWithLimitInArray)

		rV1.GET("deviceData/:devName", v1.GetDeviceData)
		rV1.POST("deviceData", v1.StoreDeviceData)
	}

	rV2 := r.Group("api/v2")
	{
		rV2.GET("sensorData", v2.GetSensorData)
	}
	r.Run(model.Settings.HttpPort)
}
