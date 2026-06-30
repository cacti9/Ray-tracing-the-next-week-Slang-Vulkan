
struct UniformBuffer {
    uint4 renderExtent;
    float deltaTime;
};
ConstantBuffer<UniformBuffer> ubo;

RWStructuredBuffer<uint> pixels;

[shader("compute")]
[numthreads(16,16,1)]
void compMain(uint3 threadId : SV_DispatchThreadID)
{
    uint width = ubo.renderExtent.x;
    uint height = ubo.renderExtent.y;
    if (threadId.x >= width || threadId.y >= height) {
        return;
    }

    uint r = uint(255.999 * (float(threadId.x) / float(width - 1)));
    uint g = uint(255.999 * (float(threadId.y) / float(height - 1)));
    uint b = 0;
    uint a = 255;
    pixels[threadId.y * width + threadId.x] = b | (g << 8) | (r << 16) | (a << 24);
}
