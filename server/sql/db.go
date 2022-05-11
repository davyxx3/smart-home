package sql

import (
	"database/sql"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
	"smart-home/middleware"
	"smart-home/model"
	"time"
)

var Db *gorm.DB

var DbPool *sql.DB

func InitDb() {
	var err error
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%s)/%s?charset=utf8&parseTime=True&loc=Local", Settings.DbUser, Settings.DbPassword, Settings.DbHost, Settings.DbPort, Settings.DbName)
	Db, err = gorm.Open(mysql.Open(dsn), &gorm.Config{})
	if err != nil {
		middleware.Logger.Fatal("Can not connect to database: ", err)
	}
	err = Db.AutoMigrate(&model.SensorData{}, &model.DeviceData{})
	if err != nil {
		middleware.Logger.Info("Can not turn on auto migration: ", err)
	}
	dbPool, err := Db.DB()
	if err != nil {
		middleware.Logger.Fatal("Can not create connection pool: ", err)
	}
	dbPool.SetMaxIdleConns(10)
	dbPool.SetMaxOpenConns(100)
	dbPool.SetConnMaxIdleTime(10 * time.Second)
}
