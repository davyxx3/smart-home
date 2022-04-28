package model

import (
	"database/sql"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
	"smart-home/middleware"
	"time"
)

var db *gorm.DB

var dbPool *sql.DB

func InitDb() {
	var err error
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%s)/%s?charset=utf8&parseTime=True&loc=Local", Settings.DbUser, Settings.DbPassword, Settings.DbHost, Settings.DbPort, Settings.DbName)
	db, err = gorm.Open(mysql.Open(dsn), &gorm.Config{})
	if err != nil {
		middleware.Logger.Fatal("Can not connect to database: ", err)
	}
	err = db.AutoMigrate(&User{}, &SensorData{}, &DeviceData{})
	if err != nil {
		middleware.Logger.Info("Can not turn on auto migration: ", err)
	}
	dbPool, err := db.DB()
	if err != nil {
		middleware.Logger.Fatal("Can not create connection pool: ", err)
	}
	dbPool.SetMaxIdleConns(10)
	dbPool.SetMaxOpenConns(100)
	dbPool.SetConnMaxIdleTime(10 * time.Second)
}
