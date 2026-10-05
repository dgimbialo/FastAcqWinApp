#include "ChirpStore.h"

ChirpStore::ChirpStore(size_t maxFrames, size_t maxBytes)
    : m_maxFrames(maxFrames > 0 ? maxFrames : 1),
      m_maxBytes(maxBytes > 0 ? maxBytes : 1)
{
}

void ChirpStore::EvictLocked()
{
    while (!m_frames.empty() &&
           (m_frames.size() > m_maxFrames ||
            (m_bytes > m_maxBytes && m_frames.size() > 1)))
    {
        m_bytes -= m_frames.front()->Bytes();
        m_frames.pop_front();
    }
}

uint64_t ChirpStore::Push(ChirpFrame&& frame)
{
    std::lock_guard<std::mutex> lock(m_mx);
    frame.seq = m_nextSeq++;
    auto p = std::make_shared<const ChirpFrame>(std::move(frame));
    m_bytes += p->Bytes();
    m_frames.push_back(p);
    EvictLocked();
    return p->seq;
}

ChirpFramePtr ChirpStore::Get(uint64_t seq) const
{
    std::lock_guard<std::mutex> lock(m_mx);
    if (m_frames.empty()) return nullptr;
    const uint64_t first = m_frames.front()->seq;
    if (seq < first) return nullptr;
    const uint64_t off = seq - first;
    if (off >= m_frames.size()) return nullptr;
    return m_frames[static_cast<size_t>(off)];
}

ChirpFramePtr ChirpStore::Latest() const
{
    std::lock_guard<std::mutex> lock(m_mx);
    return m_frames.empty() ? nullptr : m_frames.back();
}

uint64_t ChirpStore::FirstSeq() const
{
    std::lock_guard<std::mutex> lock(m_mx);
    return m_frames.empty() ? kInvalidSeq : m_frames.front()->seq;
}

uint64_t ChirpStore::NextSeq() const
{
    std::lock_guard<std::mutex> lock(m_mx);
    return m_nextSeq;
}

size_t ChirpStore::Size() const
{
    std::lock_guard<std::mutex> lock(m_mx);
    return m_frames.size();
}

size_t ChirpStore::Bytes() const
{
    std::lock_guard<std::mutex> lock(m_mx);
    return m_bytes;
}

void ChirpStore::SetLimits(size_t maxFrames, size_t maxBytes)
{
    std::lock_guard<std::mutex> lock(m_mx);
    m_maxFrames = maxFrames > 0 ? maxFrames : 1;
    m_maxBytes  = maxBytes  > 0 ? maxBytes  : 1;
    EvictLocked();
}

void ChirpStore::Clear()
{
    std::lock_guard<std::mutex> lock(m_mx);
    m_frames.clear();
    m_bytes = 0;
}
