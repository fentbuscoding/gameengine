#ifdef NEXUS_VULKAN_ENABLED

#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanResource.h"
#include "Logger.h"

namespace Nexus {
namespace RHI {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* device, VkCommandBuffer commandBuffer)
    : device_(device)
    , commandBuffer_(commandBuffer) {
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
}

void VulkanCommandBuffer::Begin() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer_, &beginInfo);
}

void VulkanCommandBuffer::End() {
    vkEndCommandBuffer(commandBuffer_);
}

void VulkanCommandBuffer::BeginRenderPass(RHIRenderPass* renderPass, RHIFramebuffer* framebuffer) {
    VulkanRenderPass* vkRenderPass = static_cast<VulkanRenderPass*>(renderPass);
    VulkanFramebuffer* vkFramebuffer = static_cast<VulkanFramebuffer*>(framebuffer);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vkRenderPass->GetVkRenderPass();
    renderPassInfo.framebuffer = vkFramebuffer->GetVkFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight()};

    VkClearValue clearValues[2];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(commandBuffer_);
}

void VulkanCommandBuffer::SetViewport(const Viewport& viewport) {
    VkViewport vkViewport{};
    vkViewport.x = viewport.x;
    vkViewport.y = viewport.y;
    vkViewport.width = viewport.width;
    vkViewport.height = viewport.height;
    vkViewport.minDepth = viewport.minDepth;
    vkViewport.maxDepth = viewport.maxDepth;

    vkCmdSetViewport(commandBuffer_, 0, 1, &vkViewport);
}

void VulkanCommandBuffer::SetScissor(const Scissor& scissor) {
    VkRect2D vkScissor{};
    vkScissor.offset = {scissor.x, scissor.y};
    vkScissor.extent = {scissor.width, scissor.height};

    vkCmdSetScissor(commandBuffer_, 0, 1, &vkScissor);
}

void VulkanCommandBuffer::SetPipeline(RHIPipeline* pipeline) {
    VulkanPipeline* vkPipeline = static_cast<VulkanPipeline*>(pipeline);
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
}

void VulkanCommandBuffer::SetVertexBuffer(RHIBuffer* buffer, uint32_t binding, uint64_t offset) {
    VulkanBuffer* vkBuffer = static_cast<VulkanBuffer*>(buffer);
    VkBuffer vertexBuffers[] = {vkBuffer->GetVkBuffer()};
    VkDeviceSize offsets[] = {offset};
    vkCmdBindVertexBuffers(commandBuffer_, binding, 1, vertexBuffers, offsets);
}

void VulkanCommandBuffer::SetIndexBuffer(RHIBuffer* buffer, uint64_t offset) {
    VulkanBuffer* vkBuffer = static_cast<VulkanBuffer*>(buffer);
    vkCmdBindIndexBuffer(commandBuffer_, vkBuffer->GetVkBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(commandBuffer_, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    vkCmdDrawIndexed(commandBuffer_, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::ClearRenderTarget(RHITexture* target, const ClearColor& color) {
    VulkanTexture* texture = static_cast<VulkanTexture*>(target);
    if (!texture || texture->GetVkImage() == VK_NULL_HANDLE) {
        Logger::Error("ClearRenderTarget called with no render target");
        return;
    }

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer = 0;
    range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = texture->GetVkImage();
    barrier.subresourceRange = range;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    // UNDEFINED as the old layout discards whatever the image held, which is
    // exactly right for a clear and avoids having to track per-image layout
    // state that this command buffer does not have.
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer_,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkClearColorValue clearValue{};
    clearValue.float32[0] = color.r;
    clearValue.float32[1] = color.g;
    clearValue.float32[2] = color.b;
    clearValue.float32[3] = color.a;

    vkCmdClearColorImage(commandBuffer_, texture->GetVkImage(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

    // A swap chain image must be in PRESENT_SRC_KHR when it reaches
    // vkQueuePresentKHR; anything else is left ready to be used as a colour
    // attachment by subsequent draws.
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    if (texture->IsSwapChainImage()) {
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(commandBuffer_,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    } else {
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer_,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}

void VulkanCommandBuffer::ClearDepthStencil(RHITexture* target, float depth, uint8_t stencil) {
    Logger::Warning("VulkanCommandBuffer::ClearDepthStencil not yet implemented");
}

void VulkanCommandBuffer::CopyBuffer(RHIBuffer* src, RHIBuffer* dst, uint64_t size, uint64_t srcOffset, uint64_t dstOffset) {
    VulkanBuffer* vkSrc = static_cast<VulkanBuffer*>(src);
    VulkanBuffer* vkDst = static_cast<VulkanBuffer*>(dst);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer_, vkSrc->GetVkBuffer(), vkDst->GetVkBuffer(), 1, &copyRegion);
}

void VulkanCommandBuffer::CopyTexture(RHITexture* src, RHITexture* dst) {
    Logger::Warning("VulkanCommandBuffer::CopyTexture not yet implemented");
}

} // namespace RHI
} // namespace Nexus

#endif // NEXUS_VULKAN_ENABLED
