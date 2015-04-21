#pragma once
//======================================================================
#include <atma/string.hpp>
#include <atma/unique_memory.hpp>
//======================================================================
namespace atma { namespace filesystem {

	struct file_t
	{
		file_t()
			: file_()
		{}

		file_t(char const* path) {
			file_ = fopen(path, "rb+");
		}

		~file_t() {
			close();
		}

		auto close() -> void
		{
			if (file_)
				fclose(file_);
			file_ = nullptr;
		}

		auto is_valid() const -> bool { return file_ != nullptr; }
		
		auto size() const -> size_t {
			auto n = ftell(file_);
			fseek(file_, 0, SEEK_END);
			auto r = ftell(file_);
			fseek(file_, n, SEEK_SET);
			return r;
		}

		auto read(void* dest, size_t size) -> void {
			fread(dest, size, 1, file_);
		}

		auto read_into_memory() -> atma::unique_memory_t
		{
			auto m = atma::unique_memory_t(size() + 1);
			read(m.begin(), size());
			*(m.begin() + size()) = '\0';
			return m;
		}

	private:
		FILE* file_;
	};

} }
