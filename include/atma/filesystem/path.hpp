#pragma once
//======================================================================
#include <atma/string.hpp>
//======================================================================
namespace atma { namespace filesystem {




#pragma message ("don't use this. I haven't needed it yet")






	struct path_t
	{
		path_t(atma::string const& str)
		{
			//detail::parse_path(nodes_, str);

			atma::string cn;
			for (auto x : str)
			{
				if (x == '\\' || x == '/')
				{
					if (!cn.empty()) {
						nodes_.push_back(cn);
						cn.clear();
					}
				}
				else {
					cn.push_back(x);
				}
			}
		}

		auto is_absolute() const -> bool { return absolute_; }
		auto is_relative() const -> bool { return !absolute_; }

	private:
		typedef std::vector<atma::string> nodes_t;
		nodes_t nodes_;
		bool absolute_;
	};

} }
