#include "Engine/Core/Runtime/AssetLoaders/CommaSeparatedValues.hpp"
#include "Engine/Core/Types/Properties/StellarClass.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

namespace
{
int Failures = 0;

void Check(bool Condition, const std::string& Message)
{
    if (!Condition)
    {
        std::cerr << "FAILED: " << Message << '\n';
        ++Failures;
    }
}

void TestStellarClassRoundTrip()
{
    using Npgs::Astro::FStellarClass;

    const FStellarClass::FSpectralType SpectralType{
        .HSpectralClass  = FStellarClass::ESpectralClass::kSpectral_G,
        .MSpectralClass  = FStellarClass::ESpectralClass::kSpectral_Unknown,
        .LuminosityClass = FStellarClass::ELuminosityClass::kLuminosity_V,
        .SpecialMark     = std::to_underlying(FStellarClass::ESpecialMark::kCode_Null),
        .Subclass        = 2.5f,
        .AmSubclass      = 0.0f,
        .bIsAmStar       = false,
    };
    const FStellarClass StellarClass(FStellarClass::EStellarType::kNormalStar, SpectralType);

    Check(StellarClass.ToString() == "G2.5V", "stellar class formatting");

    const FStellarClass Parsed = FStellarClass::Parse(StellarClass.ToString());
    const auto ParsedData = Parsed.Data();
    Check(Parsed.GetStellarType() == FStellarClass::EStellarType::kNormalStar, "stellar type round trip");
    Check(ParsedData.HSpectralClass == SpectralType.HSpectralClass, "spectral class round trip");
    Check(ParsedData.LuminosityClass == SpectralType.LuminosityClass, "luminosity class round trip");
    Check(std::abs(ParsedData.Subclass - SpectralType.Subclass) < 0.001f, "stellar subclass round trip");
}

void TestCommaSeparatedValues(const std::filesystem::path& CsvPath)
{
    {
        std::ofstream Csv(CsvPath);
        Csv << "mass,temperature,luminosity\n"
               "3,30,300\n"
               "1,10,100\n"
               "2,20,200\n";
    }

    using FCsv = Npgs::Runtime::Asset::TCommaSeparatedValues<float, 3>;
    FCsv Csv(CsvPath.string(), { "mass", "temperature", "luminosity" });

    Check(Csv.Data()->size() == 3, "CSV row count");
    Check(Csv.FindMatchingValue("mass", 2.0f, "temperature") == 20.0f, "CSV matching value");

    const auto [Lower, Upper] = Csv.FindSurroundingValues("mass", 1.5f, false);
    Check(Lower[0] == 1.0f && Upper[0] == 2.0f, "CSV surrounding values");

    bool ThrewForUnknownHeader = false;
    try
    {
        static_cast<void>(Csv.FindMatchingValue("unknown", 1.0f, "temperature"));
    }
    catch (const std::out_of_range&)
    {
        ThrewForUnknownHeader = true;
    }
    Check(ThrewForUnknownHeader, "CSV unknown header error");

    std::error_code Error;
    std::filesystem::remove(CsvPath, Error);
}
}

int main(int ArgumentCount, char** Arguments)
{
    if (ArgumentCount != 2)
    {
        std::cerr << "Expected a temporary CSV path\n";
        return 2;
    }

    TestStellarClassRoundTrip();
    TestCommaSeparatedValues(Arguments[1]);

    if (Failures != 0)
    {
        std::cerr << Failures << " core test(s) failed\n";
    }
    return Failures == 0 ? 0 : 1;
}
