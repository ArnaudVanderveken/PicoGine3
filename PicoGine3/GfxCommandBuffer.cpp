#include "pch.h"
#include "GfxCommandBuffer.h"

#include "GfxImmediateCommands.h"

GfxCommandBuffer::GfxCommandBuffer(GfxImmediateCommands* pGfxImmediateCommands) :
	m_pGfxImmediateCommands{ pGfxImmediateCommands },
	m_pWrapper{ &m_pGfxImmediateCommands->Acquire() }
{
}

const CommandBufferWrapper* GfxCommandBuffer::GetBufferWrapper() const
{
	return m_pWrapper;
}

VkCommandBuffer GfxCommandBuffer::GetCmdBuffer() const
{
	return m_pWrapper ? m_pWrapper->m_CmdBuffer : VK_NULL_HANDLE;
}
