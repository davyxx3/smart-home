package routes

import (
	"github.com/gin-gonic/gin"
	v1 "smart-home/api/v1"
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

		rV1.GET("deviceData", v1.GetDeviceData)
		rV1.POST("deviceData", v1.StoreDeviceData)
	}
	r.Run(model.Settings.HttpPort)
}
