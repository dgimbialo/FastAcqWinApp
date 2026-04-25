#include "pch.h"
#include "ChirpStore.h"

ChirpStore::ChirpStore(size_t capacity)
    : m_capacity(capacity > 0 ? capacity : 1)
{
    ::InitializeCriticalSection(&m_cs);
}

ChirpStore::~ChirpStore()
{
    ::DeleteCriticalSection(&m_cs);
}

size_t ChirpStore::Push(ChirpFrame&& frame)
{
    ::EnterCriticalSection(&m_cs);
    if (m_frames.size() >= m_capacity) {
        m_frames.pop_front();
    }
    m_frames.emplace_back(std::move(frame));
    size_t idx = m_frames.size() - 1;
    ::LeaveCriticalSection(&m_cs);
    return idx;
}

bool ChirpStore::GetAt(size_t index, ChirpFrame& out) const
{
    ::EnterCriticalSection(&m_cs);
    bool ok = false;
    if (index < m_frames.size()) {
        out = m_frames[index];
        ok = true;
    }
    ::LeaveCriticalSection(&m_cs);
    return ok;
}

bool ChirpStore::GetLatest(ChirpFrame& out) const
{
    ::EnterCriticalSection(&m_cs);
    bool ok = false;
    if (!m_frames.empty()) {
        out = m_frames.back();
        ok = true;
    }
    ::LeaveCriticalSection(&m_cs);
    return ok;
}

size_t ChirpStore::Size() const
{
    ::EnterCriticalSection(&m_cs);
    size_t n = m_frames.size();
    ::LeaveCriticalSection(&m_cs);
    return n;
}

void ChirpStore::Clear()
{
    ::EnterCriticalSection(&m_cs);
    m_frames.clear();
    ::LeaveCriticalSection(&m_cs);
}
