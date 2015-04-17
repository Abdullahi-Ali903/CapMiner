/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Worker.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Worker.h"

#include <chrono>
#include <thread>
#include "Log.h"
using namespace std;
using namespace dev;

void Worker::startWorking(IfRunning _ir)
{
//	cnote << "startWorking for thread" << m_name;
	Guard l(x_work);

	if (m_work && m_work->joinable())
		try {
			if (_ir == IfRunning::Detach)
				m_work->detach();
			else if (_ir == IfRunning::Join)
				m_work->join();
			else
				return;
		} catch (...) {}
	cnote << "Spawning" << m_name;
	m_stop = false;
	m_stopped = false;
	m_work.reset(new thread([&]()
	{
		setThreadName(m_name.c_str());
		startedWorking();
		workLoop();
		cnote << "Finishing up worker thread";
		doneWorking();
		ETH_GUARDED(x_work)
			m_work->detach();
		m_stopped = true;
	}));
}

void Worker::stopWorking()
{
//	cnote << "stopWorking for thread" << m_name;
	ETH_GUARDED(x_work)
		if (!m_work || !m_work->joinable())
			return;
	cnote << "Stopping" << m_name;
	m_stop = true;
	while (!m_stopped)
		this_thread::sleep_for(chrono::microseconds(50));
	ETH_GUARDED(x_work)
		m_work.reset();
	cnote << "Stopped" << m_name;
}

void Worker::workLoop()
{
	while (!m_stop)
	{
		if (m_idleWaitMs)
			this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
		doWork();
	}
}
