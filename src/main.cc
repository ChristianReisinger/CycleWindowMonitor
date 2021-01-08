#include <iostream>
#include <stdexcept>
#include <vector>

#include <windows.h>

struct Dimensions {
	int x, y, width, height;

	Dimensions(const RECT& r) :
			x(r.left),
			y(r.top),
			width(r.right - x),
			height(r.bottom - y) {
	}

	Dimensions(const HWND& hwnd) {
		RECT window_rect;
		GetWindowRect(hwnd, &window_rect);
		*this = Dimensions { window_rect };
	}

	Dimensions(const HMONITOR& hmon) {
		MONITORINFO monitor_info;
		monitor_info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hmon, &monitor_info))
			*this = Dimensions { monitor_info.rcMonitor };
		else
			throw std::runtime_error("failed GetMonitorInfo");
	}
};
std::ostream& operator<<(std::ostream& os, const Dimensions& dims) {
	os << "(" << dims.x << ", " << dims.y << "): " << dims.width << " x " << dims.height;
	return os;
}

struct MonitorDimensions {
	HMONITOR hmon;
	Dimensions dims;
};

void print_fgwindow_and_underlying_monitor_dimensions() {
	const auto hwnd = GetForegroundWindow();
	const Dimensions window_dims { hwnd };
	const Dimensions monitor_dims { MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST) };
	std::cout << window_dims << "\n" << monitor_dims << std::endl;
}

auto system_monitor_dimensions() {
	std::vector<MonitorDimensions> monitor_dims;
	EnumDisplayMonitors(NULL, NULL,
			[](HMONITOR hmon, HDC, LPRECT lprcmon, LPARAM dims_vec) -> BOOL {
				auto* dims = reinterpret_cast<decltype(monitor_dims)*>(dims_vec);
				dims->push_back(MonitorDimensions {
						.hmon = hmon,
						.dims = Dimensions { *lprcmon }
				});
				return TRUE;
			}, (LPARAM) &monitor_dims);

	std::sort(monitor_dims.begin(), monitor_dims.end(),
			[](const auto& lhs, const auto& rhs) {
				return lhs.dims.x < rhs.dims.x;
			});

	return monitor_dims;
}

template<typename T>
auto cycle(const std::vector<T>& v, typename std::vector<T>::const_iterator it, int steps) {
	if (it == v.end()) return it;

	if (steps < 0)
		steps = v.size() - (-steps) % v.size();
	else
		steps %= v.size();

	const auto cycled_pos = (std::distance(v.begin(), it) + steps) % v.size();
	return std::next(v.begin(), cycled_pos);
}

void cycle_fgwindow_monitor(int steps) {
	const std::vector<MonitorDimensions> system_monitor_dims = system_monitor_dimensions();

	const auto hwnd = GetForegroundWindow();
	const Dimensions window_dims { hwnd };
	const auto hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	auto window_monitor_it = std::find_if(system_monitor_dims.begin(), system_monitor_dims.end(),
			[&hmon](const MonitorDimensions& mon) {
				return mon.hmon == hmon;
			});

	const auto& first = system_monitor_dims.front();
	const auto& last = system_monitor_dims.back();
	auto relative_center_x = static_cast<double>(
		window_dims.x + window_dims.width / 2 - window_monitor_it->dims.x
		) / window_monitor_it->dims.width;

	if ((hmon == first.hmon && window_dims.x + window_dims.width < first.dims.x) ||
		(hmon == last.hmon && window_dims.x >= last.dims.x + last.dims.width)) {
		// when window fully outside desktop, move to center of outermost monitor
		if (steps < 0)
			SetWindowPos(hwnd, HWND_TOP,
					last.dims.x + (last.dims.width - window_dims.width) / 2,
					last.dims.y + (last.dims.height - window_dims.height) / 2,
					window_dims.width,
					window_dims.height,
					SWP_NOZORDER);
		else
			SetWindowPos(hwnd, HWND_TOP,
					first.dims.x + (first.dims.width - window_dims.width) / 2,
					first.dims.y + (first.dims.height - window_dims.height) / 2,
					window_dims.width,
					window_dims.height,
					SWP_NOZORDER);
	} else {
		// when more than halfway outside desktop and stepping towards outside,
		// move the non-visible part to the edge of the opposite outermost monitor
		auto relative_x = static_cast<double>(window_dims.x - window_monitor_it->dims.x) / window_monitor_it->dims.width;

		if (hmon == first.hmon && steps < 0 && relative_center_x < 0.0)
			relative_x += 1.0;
		else if (hmon == last.hmon && steps > 0 && relative_center_x > 1.0)
			relative_x -= 1.0;

		const Dimensions target_monitor_dims = cycle(system_monitor_dims, window_monitor_it, steps)->dims;
		SetWindowPos(hwnd, HWND_TOP,
				target_monitor_dims.x + target_monitor_dims.width * relative_x,
				window_dims.y,
				window_dims.width,
				window_dims.height,
				SWP_NOZORDER | SWP_NOSIZE);
	}
}

int main(int argc, char** argv) {
	if (argc == 2)
		try {
			cycle_fgwindow_monitor(std::stoi(argv[1]));
		} catch (const std::invalid_argument&) {
			std::cout << "Error: argument must be a number\n";
		} catch (const std::out_of_range& e) {
			std::cout << "Error: int out of range\n";
		}
	else
		std::cout << "Error: requires exactly 1 argument (int)\n";
}
