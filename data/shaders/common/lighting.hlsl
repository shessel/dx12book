float3 schlickFresnel(float cosTheta)
{
    float f = saturate(1.0f - cosTheta);
    float ff = f * f;
    float fffff = ff * ff * f;
    return g_cbMaterial.fresnelR0 + fffff - fffff*g_cbMaterial.fresnelR0;
}

float3 computeBrdf(LightData light, float3 positionW, float3 normalW, float3 cameraDirection, float3 lightDirection)
{

    float3 halfway = normalize(cameraDirection + lightDirection);

    float nDotL = max(dot(normalW, lightDirection), 0.0f);
    float3 ambientLight = g_cbMaterial.albedoColor.xyz * g_cbPass.ambientLight;
    float3 diffuseLight = g_cbMaterial.albedoColor.xyz * light.color;

    float m = (1.0f - g_cbMaterial.roughness) * 256.0f;
    float3 specularLight = schlickFresnel(dot(lightDirection, halfway)) * ((m + 8.0f) / 8.0f) * pow(max(dot(normalW, halfway), 0.0f), m);
    return ambientLight + nDotL * (diffuseLight + specularLight);
}

float3 computeLights(LightData lightData[MAX_LIGHT_COUNT], float3 positionW, float3 normalW, float3 toCameraNorm)
{
    float3 totalLight = (float3) 0.0f;
    uint lightIndex = 0;

    for (uint i = 0; i < g_cbPass.directionalLightCount; ++i)
    {
        LightData curLight = lightData[lightIndex];
        float3 toLight = -curLight.direction;
        float3 lightDirection = normalize(toLight);
        totalLight += computeBrdf(lightData[lightIndex], positionW, normalW, toCameraNorm, lightDirection);
        ++lightIndex;
    }

    for (uint j = 0; j < g_cbPass.pointLightCount; ++j)
    {
        LightData curLight = lightData[lightIndex];
        float3 toLight = curLight.positionW - positionW;
        float3 lightDirection = normalize(toLight);
        float distance = length(toLight);
        float falloff = 1.0f - saturate((distance - curLight.falloffBegin) / (curLight.falloffEnd - curLight.falloffBegin));
        totalLight += falloff * computeBrdf(lightData[lightIndex], positionW, normalW, toCameraNorm, lightDirection);
        ++lightIndex;
    }

    for (uint k = 0; k < g_cbPass.spotLightCount; ++k)
    {
        LightData curLight = lightData[lightIndex];
        float3 toLight = curLight.positionW - positionW;
        float3 lightDirection = normalize(toLight);
        float distance = length(toLight);
        float falloff = 1.0f - saturate((distance - curLight.falloffBegin) / (curLight.falloffEnd - curLight.falloffBegin));
        float coneFactor = max(pow(max(dot(-lightDirection, normalize(curLight.direction)), 0.0f), curLight.spotPower), 0.0f);
        totalLight += falloff * coneFactor * computeBrdf(lightData[lightIndex], positionW, normalW, toCameraNorm, lightDirection);
        ++lightIndex;
    }

    return totalLight;
}
