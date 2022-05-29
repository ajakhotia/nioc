////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "logReaderImpl.hpp"
#include <naksh/logger/logReader.hpp>

namespace naksh::logger
{


LogReader::LogReader(std::filesystem::path logRoot):
    mLogReaderImpl(std::make_unique<LogReaderImpl>(std::move(logRoot)))
{
}


LogReader::LogReader(LogReader&&) noexcept = default;


LogReader::~LogReader() = default;


LogReader& LogReader::operator=(LogReader&&) noexcept = default;


LogEntry LogReader::read()
{
    return mLogReaderImpl->read();
}


} // namespace naksh::logger
