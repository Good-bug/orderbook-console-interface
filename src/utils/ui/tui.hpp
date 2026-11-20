#pragma once
#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <chrono>
#include <exception>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "orderbook_view.hpp"

using namespace ob::orderbook;
namespace ob::utils::ui {

// Full-screen order book: run() blocks; quit() can be called from any thread.
class OrderBookApp {
   public:
    using Provider = std::function<std::shared_ptr<const BookView>()>;

    OrderBookApp(
        Provider provider, std::size_t depth = 10,
        std::chrono::milliseconds refresh = std::chrono::milliseconds(50))
        : provider_(std::move(provider)), depth_(depth), refresh_(refresh) {}

    void run() {
        auto renderer = ftxui::Renderer([&] {
            auto snap = provider_();
            if (!snap) {
                return ftxui::text("waiting for data...") | ftxui::center;
            }
            return render_book(*snap, depth_);
        });

        auto root = ftxui::CatchEvent(renderer, [&](const ftxui::Event& e) {
            if (e == ftxui::Event::Character('q') ||
                e == ftxui::Event::Escape) {
                quit();
                return true;
            }
            return false;
        });

        std::jthread ticker([&](const std::stop_token& st) {
            while (!st.stop_requested()) {
                std::this_thread::sleep_for(refresh_);
                screen_.PostEvent(ftxui::Event::Custom);
            }
        });

        screen_.Loop(root);
    }  // jthread stops and joins automatically

    void quit() { screen_.ExitLoopClosure()(); }

   private:
    Provider provider_;
    std::size_t depth_;
    std::chrono::milliseconds refresh_;
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();
};

// Runs a blocking function in a thread, catches errors, and calls on_done
// when it finishes.
class BackgroundTask {
   public:
    BackgroundTask(std::function<void()> run, std::function<void()> stop,
                   std::function<void()> on_done)
        : stop_(std::move(stop)) {
        thread_ = std::thread(
            [this, run = std::move(run), on_done = std::move(on_done)] {
                try {
                    run();
                } catch (const boost::beast::system_error& se) {
                    if (se.code() != boost::beast::websocket::error::closed &&
                        !stopping_) {
                        error_ = "system error: " + se.code().message();
                    }
                } catch (const std::exception& e) {
                    if (!stopping_) {
                        error_ = std::string("error: ") + e.what();
                    }
                }
                on_done();
            });
    }

    ~BackgroundTask() { stop_and_join(); }

    void stop_and_join() {
        if (!thread_.joinable()) {
            return;
        }
        stopping_ = true;
        stop_();
        thread_.join();
    }

    const std::optional<std::string>& error() const { return error_; }

   private:
    std::function<void()> stop_;
    std::atomic<bool> stopping_{false};
    std::optional<std::string> error_;
    std::thread thread_;
};

}  // namespace ui