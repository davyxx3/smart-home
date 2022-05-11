package model

type DataWrapper struct {
	Items []SensorData `json:"items"`
	Total int          `json:"total"`
}
