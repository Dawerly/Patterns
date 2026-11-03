import sqlite3
import time
import json
from abc import ABC, abstractmethod
from datetime import datetime, timezone
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import urlopen


def load_json(url, params):
    full_url = url + "?" + urlencode(params)

    try:
        with urlopen(full_url, timeout=10) as response:
            return json.loads(response.read().decode("utf-8"))
    except (HTTPError, URLError, TimeoutError) as error:
        raise RuntimeError("Помилка HTTP-запиту: " + str(error))
    except json.JSONDecodeError:
        raise RuntimeError("Сервіс повернув неправильний JSON")


class WeatherData:
    def __init__(self, city, latitude, longitude, retrieved_at,
                 forecast_time, temperature, precipitation, source):
        self.city = city
        self.latitude = latitude
        self.longitude = longitude
        self.retrieved_at = retrieved_at
        self.forecast_time = forecast_time
        self.temperature = temperature
        self.precipitation = precipitation
        self.source = source


class GeocodingClient:
    def __init__(self):
        self.url = "https://geocoding-api.open-meteo.com/v1/search"

    def find_city(self, city_name):
        params = {
            "name": city_name,
            "count": 1,
            "language": "uk",
            "format": "json"
        }

        data = load_json(self.url, params)

        results = data.get("results", [])
        if not results:
            raise ValueError("Місто не знайдено: " + city_name)

        city = results[0]
        return {
            "name": city.get("name", city_name),
            "latitude": city["latitude"],
            "longitude": city["longitude"],
            "country": city.get("country", "")
        }


class OpenMeteoService:
    def __init__(self):
        self.url = "https://api.open-meteo.com/v1/forecast"
        self.request_count = 0

    def request_forecast(self, latitude, longitude):
        params = {
            "latitude": latitude,
            "longitude": longitude,
            "hourly": "temperature_2m,precipitation_probability",
            "forecast_days": 1,
            "timezone": "auto"
        }

        self.request_count += 1
        return load_json(self.url, params)


class WeatherProvider(ABC):
    @abstractmethod
    def get_forecast(self, city):
        pass


class OpenMeteoAdapter(WeatherProvider):
    def __init__(self, service):
        self.service = service

    def get_forecast(self, city):
        data = self.service.request_forecast(
            city["latitude"], city["longitude"]
        )

        hourly = data.get("hourly")
        if not hourly:
            raise ValueError("У відповіді немає погодинного прогнозу")

        times = hourly.get("time", [])
        temperatures = hourly.get("temperature_2m", [])
        precipitation = hourly.get("precipitation_probability", [])

        count = min(len(times), len(temperatures), len(precipitation))
        if count == 0:
            raise ValueError("У відповіді немає погодних значень")

        retrieved_at = datetime.now(timezone.utc).isoformat(timespec="seconds")
        forecast = []

        for index in range(count):
            forecast.append(WeatherData(
                city["name"],
                city["latitude"],
                city["longitude"],
                retrieved_at,
                times[index],
                temperatures[index],
                precipitation[index],
                "Open-Meteo"
            ))

        return forecast


class ForecastCacheDecorator(WeatherProvider):
    def __init__(self, provider, ttl_seconds=600):
        self.provider = provider
        self.ttl_seconds = ttl_seconds
        self.cache = {}

    def get_forecast(self, city):
        key = (round(city["latitude"], 4), round(city["longitude"], 4))
        current_time = time.time()

        if key in self.cache:
            saved_time, forecast = self.cache[key]

            if current_time - saved_time < self.ttl_seconds:
                print("[CACHE] Використано кеш для", city["name"])
                return forecast

        print("[API] Отримано новий прогноз для", city["name"])
        forecast = self.provider.get_forecast(city)
        self.cache[key] = (current_time, forecast)
        return forecast


class RecommendationStrategy(ABC):
    @abstractmethod
    def recommend(self, forecast):
        pass


class WalkStrategy(RecommendationStrategy):
    def recommend(self, forecast):
        good_hours = []

        for weather in forecast:
            if 10 <= weather.temperature <= 25 and weather.precipitation < 40:
                good_hours.append(weather.forecast_time)

        if good_hours:
            return "Для прогулянки підійде час: " + ", ".join(good_hours[:3])

        return "Сьогодні немає зручного часу для прогулянки"


class ClothesStrategy(RecommendationStrategy):
    def recommend(self, forecast):
        average = sum(item.temperature for item in forecast) / len(forecast)

        if average < 0:
            return "Одягніть теплу зимову куртку, шапку та рукавички"
        if average < 10:
            return "Одягніть теплу куртку"
        if average < 20:
            return "Одягніть легку куртку або светр"

        return "Можна одягнути легкий літній одяг"


class RainStrategy(RecommendationStrategy):
    def recommend(self, forecast):
        max_probability = max(item.precipitation for item in forecast)

        if max_probability >= 70:
            return "Висока ймовірність опадів: візьміть парасолю та дощовик"
        if max_probability >= 40:
            return "Можливі опади: краще взяти парасолю"

        return "Значних опадів не очікується"


class RecommendationContext:
    def __init__(self, strategy):
        self.strategy = strategy

    def set_strategy(self, strategy):
        self.strategy = strategy

    def get_recommendation(self, forecast):
        return self.strategy.recommend(forecast)


class WeatherHistory:
    def __init__(self, database_name="weather_history.db"):
        self.database_name = database_name
        self.create_table()

    def create_table(self):
        try:
            with sqlite3.connect(self.database_name) as connection:
                connection.execute("""
                    CREATE TABLE IF NOT EXISTS weather_history (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        city TEXT NOT NULL,
                        latitude REAL NOT NULL,
                        longitude REAL NOT NULL,
                        retrieved_at TEXT NOT NULL,
                        forecast_time TEXT NOT NULL,
                        temperature_c REAL NOT NULL,
                        precipitation_probability INTEGER NOT NULL,
                        source TEXT NOT NULL
                    )
                """)
        except sqlite3.Error as error:
            raise RuntimeError("Помилка створення SQLite: " + str(error))

    def save(self, forecast):
        values = []

        for item in forecast:
            values.append((
                item.city,
                item.latitude,
                item.longitude,
                item.retrieved_at,
                item.forecast_time,
                item.temperature,
                item.precipitation,
                item.source
            ))

        try:
            with sqlite3.connect(self.database_name) as connection:
                connection.executemany("""
                    INSERT INTO weather_history (
                        city, latitude, longitude, retrieved_at,
                        forecast_time, temperature_c,
                        precipitation_probability, source
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, values)
        except sqlite3.Error as error:
            raise RuntimeError("Помилка збереження SQLite: " + str(error))

    def get_recent(self, limit=5):
        try:
            with sqlite3.connect(self.database_name) as connection:
                cursor = connection.execute("""
                    SELECT city, forecast_time, temperature_c,
                           precipitation_probability
                    FROM weather_history
                    ORDER BY id DESC
                    LIMIT ?
                """, (limit,))
                return cursor.fetchall()
        except sqlite3.Error as error:
            raise RuntimeError("Помилка читання SQLite: " + str(error))

    def get_by_city(self, city_name, limit=10):
        try:
            with sqlite3.connect(self.database_name) as connection:
                cursor = connection.execute("""
                    SELECT city, forecast_time, temperature_c,
                           precipitation_probability
                    FROM weather_history
                    WHERE city = ?
                    ORDER BY id DESC
                    LIMIT ?
                """, (city_name, limit))
                return cursor.fetchall()
        except sqlite3.Error as error:
            raise RuntimeError("Помилка читання SQLite: " + str(error))


class WeatherFacade:
    def __init__(self, geocoder, provider, history):
        self.geocoder = geocoder
        self.provider = provider
        self.history = history

    def get_weather(self, city_name):
        city = self.geocoder.find_city(city_name)
        forecast = self.provider.get_forecast(city)
        self.history.save(forecast)
        return forecast


def print_forecast(forecast, hours=5):
    print("\nПрогноз для", forecast[0].city)
    print("Час | Температура | Імовірність опадів")

    for item in forecast[:hours]:
        print(
            item.forecast_time,
            "|", str(item.temperature) + " °C",
            "|", str(item.precipitation) + "%"
        )


def print_history(rows):
    print("\nОстанні записи SQLite:")

    for city, forecast_time, temperature, precipitation in rows:
        print(
            city, "|", forecast_time,
            "|", str(temperature) + " °C",
            "|", str(precipitation) + "%"
        )


def run_program():
    cities = ["Lviv", "Kyiv", "Odesa"]
    cache_ttl = 600
    database_name = "weather_history.db"

    geocoder = GeocodingClient()
    service = OpenMeteoService()
    adapter = OpenMeteoAdapter(service)
    cached_provider = ForecastCacheDecorator(adapter, cache_ttl)
    history = WeatherHistory(database_name)
    facade = WeatherFacade(geocoder, cached_provider, history)

    saved_forecasts = {}

    for city in cities:
        try:
            forecast = facade.get_weather(city)
            saved_forecasts[city] = forecast
            print_forecast(forecast)
        except (RuntimeError, ValueError, KeyError) as error:
            print("[ERROR]", error)

    if cities[0] in saved_forecasts:
        forecast = saved_forecasts[cities[0]]
        context = RecommendationContext(WalkStrategy())

        print("\nРекомендації для", cities[0])
        print("Прогулянка:", context.get_recommendation(forecast))

        context.set_strategy(ClothesStrategy())
        print("Одяг:", context.get_recommendation(forecast))

        context.set_strategy(RainStrategy())
        print("Опади:", context.get_recommendation(forecast))

        try:
            facade.get_weather(cities[0])
        except (RuntimeError, ValueError, KeyError) as error:
            print("[ERROR]", error)

    print("\nФактична кількість HTTP-запитів прогнозу:",
          service.request_count)
    print_history(history.get_recent(5))


if __name__ == "__main__":
    run_program()
