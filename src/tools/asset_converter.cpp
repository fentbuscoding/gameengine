#include "AssetConverter.h"
#include "Logger.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::cout << "=== NEXUS ENGINE - UNIVERSAL ASSET CONVERTER ===" << std::endl;
    
    if (argc < 3) {
        std::cout << "Usage: NexusAssetConverter <input_file> <output_file> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported formats:" << std::endl;
        std::cout << "  Models: .fbx, .obj, .dae, .3ds, .blend, .gltf, .uasset" << std::endl;
        std::cout << "  Textures: .png, .jpg, .tga, .dds, .hdr, .exr" << std::endl;
        std::cout << "  Audio: .wav, .mp3, .ogg, .flac" << std::endl;
        std::cout << "  Animations: .fbx, .bvh, .anim" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --quality <high|medium|low>  Set conversion quality" << std::endl;
        std::cout << "  --compress                   Enable compression" << std::endl;
        std::cout << "  --optimize                   Optimize for Nexus Engine" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
    // Parse options
    Nexus::ConversionSettings settings;
    settings.quality = Nexus::ConversionQuality::High;
    settings.compress = false;
    settings.optimize = true;
    
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quality" && i + 1 < argc) {
            std::string quality = argv[++i];
            if (quality == "high") settings.quality = Nexus::ConversionQuality::High;
            else if (quality == "medium") settings.quality = Nexus::ConversionQuality::Medium;
            else if (quality == "low") settings.quality = Nexus::ConversionQuality::Low;
        } else if (arg == "--compress") {
            settings.compress = true;
        } else if (arg == "--optimize") {
            settings.optimize = true;
        }
    }
    
    Nexus::Logger::Info("Starting asset conversion...");
    Nexus::Logger::Info("Input: " + inputFile);
    Nexus::Logger::Info("Output: " + outputFile);
    
    try {
        Nexus::AssetConverter converter;
        
        if (!std::filesystem::exists(inputFile)) {
            Nexus::Logger::Error("Input file does not exist: " + inputFile);
            return 1;
        }
        
        // Detect asset type
        Nexus::AssetType assetType = converter.DetectAssetType(inputFile);
        if (assetType == Nexus::AssetType::Unknown) {
            Nexus::Logger::Error("Unsupported file format: " + inputFile);
            return 1;
        }
        
        // Convert the asset
        bool success = converter.ConvertAsset(inputFile, outputFile, assetType, settings);
        
        if (success) {
            std::cout << std::endl;
            std::cout << "✅ Asset converted successfully!" << std::endl;
            std::cout << "📁 Output: " << outputFile << std::endl;
            
            // Show conversion stats
            auto stats = converter.GetLastConversionStats();
            std::cout << "📊 Conversion Stats:" << std::endl;
            std::cout << "   Input size: " << stats.inputSize << " bytes" << std::endl;
            std::cout << "   Output size: " << stats.outputSize << " bytes" << std::endl;
            std::cout << "   Compression: " << (100.0f * (1.0f - float(stats.outputSize) / float(stats.inputSize))) << "%" << std::endl;
            std::cout << "   Time: " << stats.conversionTime << "ms" << std::endl;
            
            return 0;
        } else {
            std::cout << std::endl;
            std::cout << "❌ Failed to convert asset" << std::endl;
            std::cout << "📄 Check the log for details" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        Nexus::Logger::Error("Exception during conversion: " + std::string(e.what()));
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
}