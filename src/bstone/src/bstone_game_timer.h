/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2024 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#ifndef BSTONE_GAME_TICKER_INCLUDED
#define BSTONE_GAME_TICKER_INCLUDED

#include <cstdint>

#include <chrono>
#include <thread>

#include "bstone_atomic_flag.h"

namespace bstone {

using GameTimerTicks = std::int32_t;

// ==========================================================================

class GameTimer
{
public:
	GameTimer();
	GameTimer(const GameTimer& rhs) = delete;
	GameTimer(GameTimer&& rhs) noexcept = delete;
	void operator=(GameTimerTicks ticks);
	GameTimer& operator=(const GameTimer& rhs) = delete;
	GameTimer& operator=(GameTimer&& rhs) noexcept = delete;
	~GameTimer();

	bool is_started() const noexcept;

	void start(int frequency);
	void stop() noexcept;

	GameTimerTicks get_ticks() const;
	void set_ticks(GameTimerTicks ticks);
	void subtract_ticks(GameTimerTicks ticks);

	operator GameTimerTicks() const;

private:
	using Milliseconds = std::chrono::milliseconds;
	using Clock = std::chrono::steady_clock;
	using ClockTicks = Clock::duration::rep;
	using MtTicks = std::atomic<GameTimerTicks>;
	using Thread = std::thread;

#ifdef __3DS__
	// On the 3DS, threads on the same core do not preempt each other, so a
	// ticker thread may never run while the game loop is busy. The ticks are
	// computed from the elapsed time instead.
	bool is_started_{};
	int frequency_{};
	Clock::time_point start_time_{};
	GameTimerTicks offset_{};

	GameTimerTicks get_elapsed_ticks() const;
#endif // __3DS__

private:
	AtomicFlag mt_is_cancellation_requested_{};
	MtTicks mt_ticks_{};
	Thread thread_{};

private:
	void ensure_is_started() const;
	void set_ticks_internal(GameTimerTicks ticks);
	void increase_ticks() noexcept;
	void thread_main(int frequency) noexcept;
};

// ==========================================================================

void operator-=(GameTimer& a, GameTimerTicks b);

} // namespace bstone

#endif // BSTONE_GAME_TICKER_INCLUDED
