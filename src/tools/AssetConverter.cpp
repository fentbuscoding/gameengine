#include "AssetConverter.h"
#include "TextBlockParser.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string_view>

namespace Nexus {

AssetConverter::AssetConverter() {
    // Initialize supported formats
    supportedFormats_ = {
        // Standard formats
        ".fbx", ".obj", ".dae", ".gltf", ".glb", ".3ds", ".blend",
        ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".hdr", ".exr",
        ".wav", ".mp3", ".ogg", ".flac",
        ".json", ".xml", ".yml", ".yaml",
        
        // Source engine formats
        ".vmf", ".vmt", ".vtf", ".mdl", ".vvd", ".phy", ".ani", 
        ".pcf", ".bsp", ".vcd", ".vco", ".vpk", ".vdf"
    };
}

AssetConverter::~AssetConverter() {
    // Cleanup if needed
}

AssetType AssetConverter::DetectAssetType(const std::string& filePath) {
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Model formats
    if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".glb" || 
        ext == ".3ds" || ext == ".blend" || ext == ".mdl") {
        return AssetType::Model;
    }
    
    // Texture formats
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || 
        ext == ".dds" || ext == ".hdr" || ext == ".exr" || ext == ".vtf") {
        return AssetType::Texture;
    }
    
    // Material formats
    if (ext == ".mat" || ext == ".material" || ext == ".vmt") {
        return AssetType::Material;
    }
    
    // Sound formats
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
        return AssetType::Sound;
    }
    
    // Animation formats
    if (ext == ".anim" || ext == ".ani" || ext == ".bvh") {
        return AssetType::Animation;
    }
    
    // Map/Scene formats
    if (ext == ".vmf" || ext == ".bsp" || ext == ".scene" || ext == ".level") {
        return AssetType::Map;
    }
    
    // Package formats
    if (ext == ".vpk" || ext == ".pak" || ext == ".zip") {
        return AssetType::Package;
    }
    
    // Script formats
    if (ext == ".lua" || ext == ".py" || ext == ".js" || ext == ".vdf") {
        return AssetType::Script;
    }
    
    // Unknown
    return AssetType::Unknown;
}

SourceFormat AssetConverter::DetectSourceFormat(const std::string& filePath) {
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".vmf") return SourceFormat::VMF;
    if (ext == ".vmt") return SourceFormat::VMT;
    if (ext == ".vtf") return SourceFormat::VTF;
    if (ext == ".mdl") return SourceFormat::MDL;
    if (ext == ".vvd") return SourceFormat::VVD;
    if (ext == ".phy") return SourceFormat::PHY;
    if (ext == ".ani") return SourceFormat::ANI;
    if (ext == ".pcf") return SourceFormat::PCF;
    if (ext == ".bsp") return SourceFormat::BSP;
    if (ext == ".vcd") return SourceFormat::VCD;
    if (ext == ".vco") return SourceFormat::VCO;
    if (ext == ".vpk") return SourceFormat::VPK;
    if (ext == ".vdf") return SourceFormat::VDF;
    
    // Not a Source format
    return static_cast<SourceFormat>(-1);
}

bool AssetConverter::ConvertAsset(const std::string& inputPath, const std::string& outputPath,
                                 AssetType type, const ConversionSettings& settings) {
    // Start timing
    startTime_ = std::chrono::high_resolution_clock::now();
    
    // Initialize stats
    lastStats_ = ConversionStats{};
    lastStats_.inputFormat = std::filesystem::path(inputPath).extension().string();
    lastStats_.outputFormat = std::filesystem::path(outputPath).extension().string();
    
    // Get input file size
    if (std::filesystem::exists(inputPath)) {
        lastStats_.inputSize = std::filesystem::file_size(inputPath);
    } else {
        Logger::Error("Input file does not exist: " + inputPath);
        return false;
    }
    
    // Check if this is a Source engine format
    SourceFormat sourceFormat = DetectSourceFormat(inputPath);
    if (sourceFormat != static_cast<SourceFormat>(-1)) {
        // Handle Source-specific formats
        bool success = false;
        
        switch (sourceFormat) {
        case SourceFormat::VMF:
            success = ConvertVMF(inputPath, outputPath, settings);
            break;
        case SourceFormat::VMT:
            success = ConvertVMT(inputPath, outputPath, settings);
            break;
        case SourceFormat::VTF:
            success = ConvertVTF(inputPath, outputPath, settings);
            break;
        case SourceFormat::MDL:
            success = ConvertMDL(inputPath, outputPath, settings);
            break;
        case SourceFormat::BSP:
            success = ConvertBSP(inputPath, outputPath, settings);
            break;
        case SourceFormat::VPK:
            success = ConvertVPK(inputPath, outputPath, settings);
            break;
        default:
            Logger::Warning("Source format detected but specific converter not implemented: " + inputPath);
            success = false;
        }
        
        // Record time taken
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_).count();
        lastStats_.conversionTime = static_cast<int>(duration);
        
        // Get output file size if successful
        if (success && std::filesystem::exists(outputPath)) {
            lastStats_.outputSize = std::filesystem::file_size(outputPath);
        }
        
        return success;
    }
    
    // Handle generic conversion by asset type
    bool success = false;
    
    switch (type) {
    case AssetType::Model:
        success = ConvertModel(inputPath, outputPath, settings);
        break;
    case AssetType::Texture:
        success = ConvertTexture(inputPath, outputPath, settings);
        break;
    case AssetType::Map:
        success = ConvertMap(inputPath, outputPath, settings);
        break;
    case AssetType::Material:
    case AssetType::Sound:
    case AssetType::Animation:
    case AssetType::Package:
        Logger::Warning("Generic conversion not implemented for this asset type yet");
        success = false;
        break;
    case AssetType::Unknown:
    default:
        Logger::Error("Unknown asset type, cannot convert: " + inputPath);
        success = false;
    }
    
    // Record time taken
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_).count();
    lastStats_.conversionTime = static_cast<int>(duration);
    
    // Get output file size if successful
    if (success && std::filesystem::exists(outputPath)) {
        lastStats_.outputSize = std::filesystem::file_size(outputPath);
    }
    
    return success;
}

bool AssetConverter::ConvertVMF(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Converting Valve Map Format (VMF) to Nexus format: " + inputPath);
    
    // Ensure output directory exists
    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
    
    try {
        // Open the VMF file
        std::ifstream vmfFile(inputPath);
        if (!vmfFile) {
            Logger::Error("Failed to open VMF file: " + inputPath);
            return false;
        }
        
        std::stringstream buffer;
        buffer << vmfFile.rdbuf();
        std::string content = buffer.str();
        
        // Parse VMF structure (simplified)
        // VMF uses a format similar to Valve Data Format (VDF)
        
        // Create output scene file
        std::ofstream outFile(outputPath);
        if (!outFile) {
            Logger::Error("Failed to create output file: " + outputPath);
            return false;
        }
        
        // Write Nexus scene format header
        outFile << "// Converted from Valve Map Format (VMF)\n";
        outFile << "// Original file: " << inputPath << "\n";
        outFile << "// Conversion date: " << std::time(nullptr) << "\n\n";
        outFile << "NexusScene {\n";
        
        // Extract and convert world geometry
        const std::vector<std::string> worldBlocks = ExtractBracedBlocks(content, "world");
        if (!worldBlocks.empty()) {
            outFile << "  World {\n";
            
            // Extract brushes/solids
            const std::string& worldContent = worldBlocks.front();
            
            int brushCount = 0;
            for (const std::string& solidContent : ExtractBracedBlocks(worldContent, "solid")) {
                brushCount++;
                
                outFile << "    Brush {\n";
                outFile << "      id: " << brushCount << "\n";
                
                // Extract faces/sides
                int faceCount = 0;
                for (const std::string& sideContent : ExtractBracedBlocks(solidContent, "side")) {
                    faceCount++;
                    
                    outFile << "      Face {\n";
                    outFile << "        id: " << faceCount << "\n";
                    
                    // Extract material/texture
                    std::regex materialRegex(R"(material\s*\"(.*?)\")", std::regex::multiline);
                    std::smatch materialMatch;
                    if (std::regex_search(sideContent, materialMatch, materialRegex) && materialMatch.size() > 1) {
                        std::string material = materialMatch[1].str();
                        outFile << "        material: \"" << material << "\"\n";
                    }
                    
                    // Extract plane points
                    std::regex planeRegex(R"(plane\s*\"\((.*?)\) \((.*?)\) \((.*?)\)\")", std::regex::multiline);
                    std::smatch planeMatch;
                    if (std::regex_search(sideContent, planeMatch, planeRegex) && planeMatch.size() > 3) {
                        outFile << "        plane: [" << planeMatch[1].str() << "], ["
                                << planeMatch[2].str() << "], [" << planeMatch[3].str() << "]\n";
                    }
                    
                    outFile << "      }\n";
                }
                
                outFile << "    }\n";
            }
            
            outFile << "  }\n";
        }
        
        // Extract and convert entities
        int entityCount = 0;
        for (const std::string& entityContent : ExtractBracedBlocks(content, "entity")) {
            entityCount++;
            
            // Extract entity class/type
            std::regex classNameRegex(R"(classname\s*\"(.*?)\")", std::regex::multiline);
            std::smatch classNameMatch;
            std::string className = "unknown";
            if (std::regex_search(entityContent, classNameMatch, classNameRegex) && classNameMatch.size() > 1) {
                className = classNameMatch[1].str();
            }
            
            outFile << "  Entity {\n";
            outFile << "    id: " << entityCount << "\n";
            outFile << "    type: \"" << className << "\"\n";
            
            // Extract origin/position
            std::regex originRegex(R"(origin\s*\"(.*?)\")", std::regex::multiline);
            std::smatch originMatch;
            if (std::regex_search(entityContent, originMatch, originRegex) && originMatch.size() > 1) {
                outFile << "    position: " << originMatch[1].str() << "\n";
            }
            
            // Extract angles/rotation
            std::regex anglesRegex(R"(angles\s*\"(.*?)\")", std::regex::multiline);
            std::smatch anglesMatch;
            if (std::regex_search(entityContent, anglesMatch, anglesRegex) && anglesMatch.size() > 1) {
                outFile << "    rotation: " << anglesMatch[1].str() << "\n";
            }
            
            // Extract model if it's a prop
            std::regex modelRegex(R"(model\s*\"(.*?)\")", std::regex::multiline);
            std::smatch modelMatch;
            if (std::regex_search(entityContent, modelMatch, modelRegex) && modelMatch.size() > 1) {
                outFile << "    model: \"" << modelMatch[1].str() << "\"\n";
            }
            
            // Extract other common properties
            std::vector<std::string> commonProps = {
                "targetname", "skin", "scale", "rendercolor", "rendermode",
                "health", "spawnflags", "speed", "disableshadows"
            };
            
            for (const auto& prop : commonProps) {
                std::regex propRegex("\"" + prop + "\"\\s*\"(.*?)\"", std::regex::multiline);
                std::smatch propMatch;
                if (std::regex_search(entityContent, propMatch, propRegex) && propMatch.size() > 1) {
                    outFile << "    " << prop << ": \"" << propMatch[1].str() << "\"\n";
                }
            }
            
            outFile << "  }\n";
        }
        
        outFile << "}\n";
        outFile.close();
        
        Logger::Info("Successfully converted VMF to Nexus format: " + outputPath);
        Logger::Info("Converted " + std::to_string(entityCount) + " entities");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during VMF conversion: " + std::string(e.what()));
        return false;
    }
}

bool AssetConverter::ConvertVMT(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Converting Valve Material Type (VMT) to Nexus material: " + inputPath);
    
    // Ensure output directory exists
    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
    
    try {
        // Parse VMT file
        SourceMaterialParams materialParams = ParseVMT(inputPath);
        
        // Create output material file
        std::ofstream outFile(outputPath);
        if (!outFile) {
            Logger::Error("Failed to create output file: " + outputPath);
            return false;
        }
        
        // Write Nexus material format
        outFile << "// Converted from Valve Material Type (VMT)\n";
        outFile << "// Original file: " << inputPath << "\n";
        outFile << "// Conversion date: " << std::time(nullptr) << "\n\n";
        outFile << "{\n";
        outFile << "  \"type\": \"material\",\n";
        outFile << "  \"shader\": \"" << (materialParams.shader.empty() ? "PBR" : materialParams.shader) << "\",\n";
        outFile << "  \"properties\": {\n";
        
        // Base texture
        if (!materialParams.baseTexture.empty()) {
            outFile << "    \"albedoMap\": \"" << materialParams.baseTexture << "\",\n";
        }
        
        // Normal map
        if (!materialParams.bumpMap.empty()) {
            outFile << "    \"normalMap\": \"" << materialParams.bumpMap << "\",\n";
            outFile << "    \"normalScale\": " << materialParams.bumpScale << ",\n";
        }
        
        // Other common properties
        outFile << "    \"roughness\": " << (1.0f - materialParams.glossiness) << ",\n";
        outFile << "    \"metallic\": " << materialParams.specularLevel << ",\n";
        outFile << "    \"opacity\": " << materialParams.opacity << ",\n";
        outFile << "    \"transparent\": " << (materialParams.translucent ? "true" : "false") << ",\n";
        
        // Custom parameters
        for (const auto& [key, value] : materialParams.customParams) {
            outFile << "    \"" << key << "\": \"" << value << "\",\n";
        }
        
        // Close properties and file
        outFile << "  }\n";
        outFile << "}\n";
        
        outFile.close();
        Logger::Info("Successfully converted VMT to Nexus material: " + outputPath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during VMT conversion: " + std::string(e.what()));
        return false;
    }
}

bool AssetConverter::ConvertVTF(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Converting Valve Texture Format (VTF) to standard texture: " + inputPath);
    
    // Note: Converting VTF requires a specialized VTF library
    // For this example, we'll implement a placeholder that would need to be completed
    Logger::Warning("VTF conversion requires VTFLib or similar library");
    
    // Determine output format if not specified
    std::string outputFormat = settings.outputFormat;
    if (outputFormat.empty()) {
        // Default to DDS for best compatibility with Nexus Engine
        outputFormat = ".dds";
        
        // Make sure output path has correct extension
        std::filesystem::path outPath(outputPath);
        if (outPath.extension() != outputFormat) {
            outPath.replace_extension(outputFormat);
        }
    }
    
    // TODO: Implement actual VTF conversion using a library like VTFLib
    // This would involve:
    // 1. Loading the VTF file
    // 2. Extracting image data and mipmaps
    // 3. Converting to the target format
    // 4. Saving to the output path
    
    Logger::Error("VTF conversion not fully implemented yet");
    lastStats_.errors++;
    return false;
}

bool AssetConverter::ConvertMDL(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Converting Source Model (MDL) to Nexus model format: " + inputPath);
    
    // Note: Converting MDL requires specialized Source engine model parsing
    // This is a placeholder for the actual implementation
    
    try {
        // Parse MDL file to get metadata
        SourceModelInfo modelInfo = ParseMDL(inputPath);
        
        // Ensure output directory exists
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
        
        // For now, just write metadata file as a stub
        std::ofstream outFile(outputPath);
        if (!outFile) {
            Logger::Error("Failed to create output file: " + outputPath);
            return false;
        }
        
        // Write model metadata file
        outFile << "// Converted from Source MDL\n";
        outFile << "// Original file: " << inputPath << "\n";
        outFile << "// Conversion date: " << std::time(nullptr) << "\n\n";
        outFile << "{\n";
        outFile << "  \"type\": \"model\",\n";
        outFile << "  \"format\": \"nexus_model\",\n";
        outFile << "  \"materials\": [\n";
        
        // List materials
        for (const auto& material : modelInfo.materialPaths) {
            outFile << "    \"" << material << "\",\n";
        }
        
        outFile << "  ],\n";
        outFile << "  \"lods\": " << modelInfo.lodCount << ",\n";
        outFile << "  \"hasCollision\": " << (modelInfo.hasCollision ? "true" : "false") << ",\n";
        
        if (!modelInfo.physicsModel.empty()) {
            outFile << "  \"physicsModel\": \"" << modelInfo.physicsModel << "\",\n";
        }
        
        if (!modelInfo.animations.empty()) {
            outFile << "  \"animations\": [\n";
            for (const auto& anim : modelInfo.animations) {
                outFile << "    \"" << anim << "\",\n";
            }
            outFile << "  ],\n";
        }
        
        outFile << "  \"sourceFile\": \"" << inputPath << "\"\n";
        outFile << "}\n";
        
        outFile.close();
        
        Logger::Warning("MDL conversion stub created. Full conversion requires Source SDK implementation.");
        lastStats_.warnings++;
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during MDL conversion: " + std::string(e.what()));
        return false;
    }
}

bool AssetConverter::ConvertBSP(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Converting Source BSP level to Nexus scene format: " + inputPath);
    
    // Note: Converting BSP requires specialized Source engine level parsing
    // This is a placeholder for the actual implementation
    
    Logger::Warning("BSP conversion requires Source SDK implementation");
    lastStats_.warnings++;
    
    // TODO: Implement BSP parsing and conversion
    // This would involve:
    // 1. Reading the BSP header and verifying format
    // 2. Extracting geometry, materials, entities, etc.
    // 3. Converting to Nexus scene format
    // 4. Writing to output file
    
    Logger::Error("BSP conversion not fully implemented yet");
    lastStats_.errors++;
    return false;
}

bool AssetConverter::ConvertVPK(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    Logger::Info("Extracting Valve Package (VPK) to directory: " + inputPath);
    
    // For packages, output should be a directory
    std::filesystem::path outPath(outputPath);
    if (!outPath.has_extension()) {
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outPath);
        
        // Call extract function
        return ExtractVPK(inputPath, outputPath);
    } else {
        Logger::Error("VPK extraction requires directory as output path, not file");
        return false;
    }
}

bool AssetConverter::ExtractVPK(const std::string& vpkPath, const std::string& outputDir) {
    // This is a placeholder for VPK extraction
    // In a real implementation, you would use a VPK library to extract files
    
    Logger::Warning("VPK extraction requires specialized library");
    lastStats_.warnings++;
    
    // TODO: Implement VPK extraction
    // This would involve:
    // 1. Reading VPK header
    // 2. Parsing directory structure
    // 3. Extracting files to output directory
    // 4. Optional: converting extracted files to Nexus formats
    
    Logger::Error("VPK extraction not fully implemented yet");
    lastStats_.errors++;
    return false;
}

SourceMaterialParams AssetConverter::ParseVMT(const std::string& vmtPath) {
    SourceMaterialParams params;
    
    try {
        // Open VMT file
        std::ifstream vmtFile(vmtPath);
        if (!vmtFile) {
            Logger::Error("Failed to open VMT file: " + vmtPath);
            return params;
        }
        
        std::stringstream buffer;
        buffer << vmtFile.rdbuf();
        std::string content = buffer.str();
        
        // Extract shader name (first token in file)
        std::regex shaderRegex(R"(^\s*\"([^\"]+)\")", std::regex::multiline);
        std::smatch shaderMatch;
        if (std::regex_search(content, shaderMatch, shaderRegex) && shaderMatch.size() > 1) {
            params.shader = shaderMatch[1].str();
        }
        
        // Extract common parameters
        
        // Base texture
        std::regex baseTextureRegex(R"(\$basetexture\s+\"([^\"]+)\")", std::regex::icase);
        std::smatch baseTextureMatch;
        if (std::regex_search(content, baseTextureMatch, baseTextureRegex) && baseTextureMatch.size() > 1) {
            params.baseTexture = baseTextureMatch[1].str();
        }
        
        // Normal map
        std::regex bumpMapRegex(R"(\$bumpmap\s+\"([^\"]+)\")", std::regex::icase);
        std::smatch bumpMapMatch;
        if (std::regex_search(content, bumpMapMatch, bumpMapRegex) && bumpMapMatch.size() > 1) {
            params.bumpMap = bumpMapMatch[1].str();
        }
        
        // Bump scale
        std::regex bumpScaleRegex(R"(\$bumpscale\s+([0-9\.]+))", std::regex::icase);
        std::smatch bumpScaleMatch;
        if (std::regex_search(content, bumpScaleMatch, bumpScaleRegex) && bumpScaleMatch.size() > 1) {
            params.bumpScale = std::stof(bumpScaleMatch[1].str());
        }
        
        // Translucent flag
        std::regex translucentRegex(R"(\$translucent\s+([0-9]+))", std::regex::icase);
        std::smatch translucentMatch;
        if (std::regex_search(content, translucentMatch, translucentRegex) && translucentMatch.size() > 1) {
            params.translucent = (std::stoi(translucentMatch[1].str()) > 0);
        }
        
        // Opacity/Alpha
        std::regex opacityRegex(R"(\$alpha\s+([0-9\.]+))", std::regex::icase);
        std::smatch opacityMatch;
        if (std::regex_search(content, opacityMatch, opacityRegex) && opacityMatch.size() > 1) {
            params.opacity = std::stof(opacityMatch[1].str());
        }
        
        // Extract all other parameters to custom params map
        std::regex paramRegex(R"(\$([a-zA-Z0-9_]+)\s+\"?([^\"]+)\"?)", std::regex::multiline);
        std::smatch paramMatch;
        std::string::const_iterator searchStart(content.cbegin());
        
        while (std::regex_search(searchStart, content.cend(), paramMatch, paramRegex)) {
            if (paramMatch.size() > 2) {
                std::string paramName = paramMatch[1].str();
                std::string paramValue = paramMatch[2].str();
                
                // Skip already processed parameters
                if (paramName != "basetexture" && paramName != "bumpmap" &&
                    paramName != "bumpscale" && paramName != "translucent" && paramName != "alpha") {
                    params.customParams[paramName] = paramValue;
                }
            }
            
            searchStart = paramMatch.suffix().first;
        }
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during VMT parsing: " + std::string(e.what()));
    }
    
    return params;
}

SourceModelInfo AssetConverter::ParseMDL(const std::string& mdlPath) {
    // This is a placeholder for MDL parsing
    // In a real implementation, you would parse the binary MDL format
    SourceModelInfo info;
    
    try {
        // This would require reading binary MDL header and structures
        // For placeholder purposes, we'll fake some data
        info.materialPaths.push_back("materials/placeholder.vmt");
        info.lodCount = 1;
        info.hasCollision = true;
        
        // Model filename without extension
        std::filesystem::path path(mdlPath);
        std::string baseName = path.stem().string();
        
        // Related files
        info.physicsModel = path.parent_path().string() + "/" + baseName + ".phy";
        info.animations.push_back(path.parent_path().string() + "/" + baseName + "_idle.ani");
        info.animations.push_back(path.parent_path().string() + "/" + baseName + "_walk.ani");
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during MDL parsing: " + std::string(e.what()));
    }
    
    return info;
}

bool AssetConverter::BatchConvertSourceGame(const std::string& gamePath, const std::string& outputDir,
                                          const ConversionSettings& settings) {
    Logger::Info("Batch converting Source engine game content: " + gamePath);
    
    // Check if the source game path is valid
    if (!CheckSourceGameValid(gamePath)) {
        Logger::Error("Invalid Source game directory: " + gamePath);
        return false;
    }
    
    // Create output directory
    std::filesystem::create_directories(outputDir);
    
    // Track progress
    int totalFiles = 0;
    int convertedFiles = 0;
    int errorFiles = 0;
    
    try {
        // Find and convert all VPK files first
        for (const auto& entry : std::filesystem::recursive_directory_iterator(gamePath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".vpk") {
                std::string vpkPath = entry.path().string();
                std::string vpkName = entry.path().stem().string();
                std::string extractDir = outputDir + "/" + vpkName;
                
                totalFiles++;
                if (ExtractVPK(vpkPath, extractDir)) {
                    convertedFiles++;
                } else {
                    errorFiles++;
                }
            }
        }
        
        // Find and convert models, materials, and maps
        std::vector<std::pair<std::string, std::string>> conversions;
        
        // First collect all files to convert
        for (const auto& entry : std::filesystem::recursive_directory_iterator(gamePath)) {
            if (!entry.is_regular_file()) continue;
            
            const auto& path = entry.path();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".mdl" || ext == ".vmt" || ext == ".vtf" || ext == ".bsp") {
                totalFiles++;
                
                // Create relative path for output
                std::filesystem::path relativePath = std::filesystem::relative(path, gamePath);
                std::filesystem::path outputPath = outputDir / relativePath;
                
                // Change extension based on output format
                if (ext == ".vtf") {
                    outputPath.replace_extension(".dds");
                } else if (ext == ".mdl") {
                    outputPath.replace_extension(".fbx");
                } else if (ext == ".vmt") {
                    outputPath.replace_extension(".mat");
                } else if (ext == ".bsp") {
                    outputPath.replace_extension(".scene");
                }
                
                // Ensure parent directory exists
                std::filesystem::create_directories(outputPath.parent_path());
                
                // Add to conversion list
                conversions.push_back({path.string(), outputPath.string()});
            }
        }
        
        // Now perform conversions
        for (const auto& [srcPath, dstPath] : conversions) {
            AssetType type = DetectAssetType(srcPath);
            if (ConvertAsset(srcPath, dstPath, type, settings)) {
                convertedFiles++;
            } else {
                errorFiles++;
            }
        }
        
        Logger::Info("Batch conversion complete:");
        Logger::Info("  Total files: " + std::to_string(totalFiles));
        Logger::Info("  Converted: " + std::to_string(convertedFiles));
        Logger::Info("  Errors: " + std::to_string(errorFiles));
        
        return errorFiles == 0;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during batch conversion: " + std::string(e.what()));
        return false;
    }
}

bool AssetConverter::CheckSourceGameValid(const std::string& gamePath) {
    // Check for common Source engine directories and files
    std::vector<std::string> requiredDirs = {
        "materials",
        "models",
        "maps"
    };
    
    for (const auto& dir : requiredDirs) {
        if (!std::filesystem::exists(gamePath + "/" + dir)) {
            return false;
        }
    }
    
    // Check for gameinfo.txt
    if (!std::filesystem::exists(gamePath + "/gameinfo.txt")) {
        return false;
    }
    
    return true;
}

bool AssetConverter::ConvertTexture(const std::string& inputPath, const std::string& outputPath, 
                                  const ConversionSettings& settings) {
    // Generic texture conversion
    Logger::Info("Converting texture: " + inputPath);
    
    // TODO: Implement actual texture conversion
    // This would typically use a library like stb_image, DevIL, etc.
    
    Logger::Warning("Generic texture conversion not fully implemented");
    lastStats_.warnings++;
    return false;
}

bool AssetConverter::ConvertModel(const std::string& inputPath, const std::string& outputPath, 
                                const ConversionSettings& settings) {
    // Generic model conversion
    Logger::Info("Converting model: " + inputPath);
    
    // TODO: Implement actual model conversion
    // This would typically use a library like Assimp, FBX SDK, etc.
    
    Logger::Warning("Generic model conversion not fully implemented");
    lastStats_.warnings++;
    return false;
}

bool AssetConverter::ConvertMap(const std::string& inputPath, const std::string& outputPath, 
                              const ConversionSettings& settings) {
    // Generic map/level conversion
    Logger::Info("Converting map/level: " + inputPath);
    
    // TODO: Implement actual map conversion
    // This would depend on the specific format
    
    Logger::Warning("Generic map conversion not fully implemented");
    lastStats_.warnings++;
    return false;
}

bool AssetConverter::BatchConvertDirectory(const std::string& inputDir, const std::string& outputDir, 
                                         const ConversionSettings& settings) {
    Logger::Info("Batch converting directory: " + inputDir);
    
    if (!std::filesystem::exists(inputDir)) {
        Logger::Error("Input directory does not exist: " + inputDir);
        return false;
    }
    
    // Create output directory
    std::filesystem::create_directories(outputDir);
    
    // Track progress
    int totalFiles = 0;
    int convertedFiles = 0;
    int errorFiles = 0;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
            if (!entry.is_regular_file()) continue;
            
            const auto& path = entry.path();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // Check if it's a supported format
            if (std::find(supportedFormats_.begin(), supportedFormats_.end(), ext) != supportedFormats_.end()) {
                totalFiles++;
                
                // Create relative path for output
                std::filesystem::path relativePath = std::filesystem::relative(path, inputDir);
                std::filesystem::path outputPath = outputDir / relativePath;
                
                // Ensure parent directory exists
                std::filesystem::create_directories(outputPath.parent_path());
                
                // Convert the asset
                AssetType type = DetectAssetType(path.string());
                if (ConvertAsset(path.string(), outputPath.string(), type, settings)) {
                    convertedFiles++;
                } else {
                    errorFiles++;
                }
            }
        }
        
        Logger::Info("Batch conversion complete:");
        Logger::Info("  Total files: " + std::to_string(totalFiles));
        Logger::Info("  Converted: " + std::to_string(convertedFiles));
        Logger::Info("  Errors: " + std::to_string(errorFiles));
        
        return errorFiles == 0;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception during batch conversion: " + std::string(e.what()));
        return false;
    }
}

ConversionStats AssetConverter::GetLastConversionStats() const {
    return lastStats_;
}

} // namespace Nexus