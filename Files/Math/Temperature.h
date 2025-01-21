#pragma once

namespace Volume{
    struct Temperature {
	private:
		float Temperature_C;  // Temperature in Celsius

		static constexpr float KELVIN_OFFSET = 273.15f;
		static constexpr float FAHRENHEIT_OFFSET = 32.0f;
		static constexpr float FAHRENHEIT_RATIO = 9.0f / 5.0f;
		static constexpr float FAHRENHEIT_INVERSE_RATIO = 5.0f / 9.0f;
	public:
		float GetKelvin() const { return Temperature_C + KELVIN_OFFSET; }
		void SetKelvin(float kelvin) { Temperature_C = kelvin - KELVIN_OFFSET; }
		float GetFahrenheit() const { return Temperature_C * FAHRENHEIT_RATIO + FAHRENHEIT_OFFSET; }
		void SetFahrenheit(float fahrenheit) { Temperature_C = (fahrenheit - FAHRENHEIT_OFFSET) * FAHRENHEIT_INVERSE_RATIO; }
		float GetCelsius() const { return Temperature_C; }
		void SetCelsius(float celsius) { Temperature_C = celsius; }

		Temperature() : Temperature_C(0) {}
		Temperature(float celsius) : Temperature_C(celsius) {}
    };
}
