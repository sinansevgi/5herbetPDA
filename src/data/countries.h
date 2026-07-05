#pragma once
#include <Arduino.h>

struct CountryData {
    const char* name;
    const char* capital;
    const char* population;
    const char* currency;
    int timezoneOffset;
    int x;
    int y;
};

const CountryData DEFAULT_COUNTRIES[] = {
    {"Afghanistan", "Kabul", "38M", "AFN", 4, 210, 75},
    {"Albania", "Tirana", "2.8M", "ALL", 1, 169, 66},
    {"Algeria", "Algiers", "43M", "DZD", 1, 155, 72},
    {"Argentina", "Buenos Aires", "45M", "ARS", -3, 103, 171},
    {"Australia", "Canberra", "25M", "AUD", 10, 278, 172},
    {"Austria", "Vienna", "8.9M", "EUR", 1, 166, 56},
    {"Bangladesh", "Dhaka", "164M", "BDT", 6, 228, 90},
    {"Belgium", "Brussels", "11M", "EUR", 1, 156, 52},
    {"Brazil", "Brasilia", "212M", "BRL", -3, 112, 145},
    {"Canada", "Ottawa", "38M", "CAD", -5, 88, 60},
    {"Chile", "Santiago", "19M", "CLP", -4, 93, 169},
    {"China", "Beijing", "1402M", "CNY", 8, 250, 68},
    {"Colombia", "Bogota", "50M", "COP", -5, 90, 117},
    {"Czech Republic", "Prague", "10.7M", "CZK", 1, 173, 53},
    {"Denmark", "Copenhagen", "5.8M", "DKK", 1, 163, 46},
    {"Egypt", "Cairo", "102M", "EGP", 2, 179, 81},
    {"Ethiopia", "Addis Ababa", "114M", "ETB", 3, 185, 110},
    {"Finland", "Helsinki", "5.5M", "EUR", 2, 173, 39},
    {"France", "Paris", "67M", "EUR", 1, 154, 55},
    {"Germany", "Berlin", "83M", "EUR", 1, 163, 50},
    {"Greece", "Athens", "10.4M", "EUR", 2, 172, 70},
    {"India", "New Delhi", "1380M", "INR", 5, 217, 83},
    {"Indonesia", "Jakarta", "273M", "IDR", 7, 242, 132},
    {"Iran", "Tehran", "83M", "IRR", 3, 196, 73},
    {"Iraq", "Baghdad", "40M", "IQD", 3, 190, 77},
    {"Ireland", "Dublin", "4.9M", "EUR", 0, 147, 49},
    {"Israel", "Jerusalem", "9.2M", "ILS", 2, 182, 79},
    {"Italy", "Rome", "60M", "EUR", 1, 163, 65},
    {"Japan", "Tokyo", "125M", "JPY", 9, 270, 73},
    {"Kenya", "Nairobi", "53M", "KES", 3, 183, 125},
    {"Malaysia", "Kuala Lumpur", "32M", "MYR", 8, 238, 119},
    {"Mexico", "Mexico City", "128M", "MXN", -6, 69, 96},
    {"Morocco", "Rabat", "36M", "MAD", 1, 146, 76},
    {"Netherlands", "Amsterdam", "17M", "EUR", 1, 156, 50},
    {"New Zealand", "Wellington", "5M", "NZD", 12, 300, 180},
    {"Nigeria", "Abuja", "206M", "NGN", 1, 159, 110},
    {"Norway", "Oslo", "5.4M", "NOK", 1, 161, 40},
    {"Pakistan", "Islamabad", "220M", "PKR", 5, 214, 76},
    {"Peru", "Lima", "32M", "PEN", -5, 87, 140},
    {"Philippines", "Manila", "109M", "PHP", 8, 254, 103},
    {"Poland", "Warsaw", "38M", "PLN", 1, 170, 50},
    {"Portugal", "Lisbon", "10.3M", "EUR", 0, 145, 69},
    {"Romania", "Bucharest", "19M", "RON", 2, 182, 58},
    {"Russia", "Moscow", "144M", "RUB", 3, 184, 46},
    {"Saudi Arabia", "Riyadh", "34M", "SAR", 3, 192, 89},
    {"South Africa", "Pretoria", "59M", "ZAR", 2, 176, 159},
    {"South Korea", "Seoul", "51M", "KRW", 9, 259, 71},
    {"Spain", "Madrid", "47M", "EUR", 1, 149, 67},
    {"Sweden", "Stockholm", "10M", "SEK", 1, 167, 41},
    {"Switzerland", "Bern", "8.6M", "CHF", 1, 158, 58},
    {"Thailand", "Bangkok", "69M", "THB", 7, 237, 104},
    {"Turkey", "Ankara", "84M", "TRY", 3, 180, 68},
    {"UK", "London", "67M", "GBP", 0, 156, 46},
    {"Ukraine", "Kyiv", "44M", "UAH", 2, 178, 53},
    {"USA", "Washington D.C.", "331M", "USD", -5, 74, 70},
    {"Vietnam", "Hanoi", "97M", "VND", 7, 241, 94},
};

const int ATLAS_COUNTRY_COUNT = 56;
