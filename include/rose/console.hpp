#pragma once

namespace rose
{
	struct console_t
	{
	private:
		console_t();

		friend struct runtime_t;
	};
}
