/*
 * COPYRIGHT (C) 2017-2021, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ASIO2_TCP_KEEPALIVE_COMPONENT_HPP__
#define __ASIO2_TCP_KEEPALIVE_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <tuple>

#include <asio2/bho/predef.h>

#include <asio2/base/error.hpp>

#include <asio2/base/detail/util.hpp>

#if BHO_OS_WINDOWS
#	if __has_include(<Mstcpip.h>)
#		include <Mstcpip.h> // tcp_keepalive struct
#	endif
#endif

namespace asio2::detail
{
	template<class derived_t, class args_t>
	class tcp_keepalive_cp
	{
	public:
		 tcp_keepalive_cp() noexcept {}
		~tcp_keepalive_cp() noexcept {}

		/**
		 * @brief set tcp socket keep alive options
		 * @param onoff    - Turn keepalive on or off.
		 * @param idle     - How many seconds after the connection is idle, start sending keepalives.
		 * @param interval - How many seconds later to send again when no reply is received.
		 * @param count    - How many times to resend when no reply is received.
		 * @li on macOS Catalina 10.15.5 (19F101), the default value is:
		 * onoff - false, idle - 7200, interval - 75, count - 8
		 */
		bool set_keep_alive_options(
			bool         onoff = true,
			unsigned int idle = 60,
			unsigned int interval = 3,
			unsigned int count = 3
		) noexcept
		{
			try
			{
				derived_t& derive = static_cast<derived_t&>(*this);

				auto & socket = derive.socket().lowest_layer();
				if (!socket.is_open())
				{
					set_last_error(asio::error::not_connected);
					return false;
				}

				asio::socket_base::keep_alive option(onoff);
				socket.set_option(option);

				auto native_fd = socket.native_handle();

				detail::ignore_unused(onoff, idle, interval, count, native_fd);

			#if BHO_OS_LINUX
				// For *n*x systems
				int ret_keepidle  = setsockopt(native_fd, SOL_TCP, TCP_KEEPIDLE , (void*)&idle    , sizeof(unsigned int));
				if (ret_keepidle)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}

				int ret_keepintvl = setsockopt(native_fd, SOL_TCP, TCP_KEEPINTVL, (void*)&interval, sizeof(unsigned int));
				if (ret_keepintvl)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}

				int ret_keepcount = setsockopt(native_fd, SOL_TCP, TCP_KEEPCNT  , (void*)&count   , sizeof(unsigned int));
				if (ret_keepcount)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}
			#elif BHO_OS_UNIX
				// be pending
			#elif BHO_OS_MACOS
				int ret_keepalive = setsockopt(native_fd, IPPROTO_TCP, TCP_KEEPALIVE, (void*)&idle    , sizeof(unsigned int));
				if (ret_keepalive)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}

				int ret_keepintvl = setsockopt(native_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&interval, sizeof(unsigned int));
				if (ret_keepintvl)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}

				int ret_keepcount = setsockopt(native_fd, IPPROTO_TCP, TCP_KEEPCNT  , (void*)&count   , sizeof(unsigned int));
				if (ret_keepcount)
				{
					set_last_error(errno, asio::error::get_system_category());
					return false;
				}
			#elif BHO_OS_IOS
				// be pending
			#elif BHO_OS_WINDOWS
				// Partially supported on windows
				tcp_keepalive keepalive_options;
				keepalive_options.onoff = onoff;
				keepalive_options.keepalivetime = idle * 1000; // Keep Alive in milliseconds.
				keepalive_options.keepaliveinterval = interval * 1000; // Resend if No-Reply 

				DWORD bytes_returned = 0;

				if (SOCKET_ERROR == ::WSAIoctl(native_fd, SIO_KEEPALIVE_VALS, (LPVOID)&keepalive_options,
					(DWORD)sizeof(keepalive_options), nullptr, 0, (LPDWORD)&bytes_returned, nullptr, nullptr))
				{
					if (::WSAGetLastError() != WSAEWOULDBLOCK)
					{
						set_last_error(::WSAGetLastError(), asio::error::get_system_category());
						return false;
					}
				}
			#endif
				return true;
			}
			catch (system_error & e)
			{
				set_last_error(e);
			}
			return false;
		}

	private:
	};
}

#endif // !__ASIO2_TCP_KEEPALIVE_COMPONENT_HPP__
