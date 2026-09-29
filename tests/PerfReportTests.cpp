#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "perf/Report.h"

namespace perf = f4cf::perf;
using namespace std::chrono_literals;

namespace
{
    const perf::Report::Node* findIn(const std::vector<perf::Report::Node>& nodes, const perf::Site& site)
    {
        for (const auto& node : nodes) {
            if (node.site == &site) {
                return &node;
            }
            if (const auto* found = findIn(node.children, site)) {
                return found;
            }
        }
        return nullptr;
    }

    const perf::Report::Node* find(const perf::Report& report, const perf::Site& site)
    {
        for (const auto& thread : report.threads) {
            if (const auto* found = findIn(thread.roots, site)) {
                return found;
            }
        }
        return nullptr;
    }

    int countIn(const std::vector<perf::Report::Node>& nodes, const perf::Site& site)
    {
        int count = 0;
        for (const auto& node : nodes) {
            count += (node.site == &site ? 1 : 0) + countIn(node.children, site);
        }
        return count;
    }

    int count(const perf::Report& report, const perf::Site& site)
    {
        int total = 0;
        for (const auto& thread : report.threads) {
            total += countIn(thread.roots, site);
        }
        return total;
    }

    const perf::Report::Thread* threadOf(const perf::Report& report, const perf::Site& root)
    {
        for (const auto& thread : report.threads) {
            for (const auto& node : thread.roots) {
                if (node.site == &root) {
                    return &thread;
                }
            }
        }
        return nullptr;
    }
}

TEST_CASE("Perf report: a site is a child of the site it runs inside")
{
    perf::setEnabled(true);
    static perf::Site outer("ReportTest::outer", nullptr);
    static perf::Site inner("ReportTest::outer", "inner");
    {
        const perf::Scope outerScope(outer);
        const perf::Scope innerScope(inner);
    }

    const auto report = perf::readReport();
    const auto* node = find(report, outer);
    REQUIRE(node != nullptr);
    REQUIRE(threadOf(report, outer) != nullptr);
    REQUIRE(node->stats.durations.count == 1);
    REQUIRE(node->children.size() == 1);
    REQUIRE(node->children[0].site == &inner);
    REQUIRE(node->children[0].stats.durations.count == 1);
}

TEST_CASE("Perf report: a site that recorded nothing in the window is left out, unless something under it recorded")
{
    perf::setEnabled(true);
    static perf::Site idle("ReportTest::idle", nullptr);
    static perf::Site open("ReportTest::open", nullptr);
    static perf::Site child("ReportTest::open", "child");
    {
        const perf::Scope scope(idle);
    }
    {
        const perf::Scope openScope(open);
        perf::reset();
        {
            const perf::Scope childScope(child);
        }
        // read while open is still running: it has recorded nothing yet, but its child has
        const auto report = perf::readReport();
        REQUIRE(find(report, idle) == nullptr);
        const auto* node = find(report, open);
        REQUIRE(node != nullptr);
        REQUIRE(node->stats.durations.count == 0);
        REQUIRE(node->children.size() == 1);
        REQUIRE(node->children[0].site == &child);
    }
}

TEST_CASE("Perf report: the frame site's thread is the game thread, and it counts the frames")
{
    perf::setEnabled(true);
    auto& frame = perf::declareFrameSite("ReportTest::frame");
    REQUIRE(perf::frameSite() == &frame);
    REQUIRE(&perf::declareFrameSite("ignored: the first call named it") == &frame);

    static perf::Site worker("ReportTest::worker", nullptr);
    perf::reset();
    for (int i = 0; i < 3; ++i) {
        const perf::Scope frameScope(frame);
    }
    std::thread([] {
        const perf::Scope workerScope(worker);
    }).join();

    const auto report = perf::readReport();
    REQUIRE(report.frames == 3);
    REQUIRE_FALSE(report.threads.empty());
    REQUIRE(report.threads[0].isGameThread);
    REQUIRE(report.threads[0].id == std::this_thread::get_id());
    REQUIRE(threadOf(report, frame) == &report.threads[0]);

    const auto* workerThread = threadOf(report, worker);
    REQUIRE(workerThread != nullptr);
    REQUIRE_FALSE(workerThread->isGameThread);
    REQUIRE(workerThread->id != std::this_thread::get_id());
}

TEST_CASE("Perf report: a caller loop is cut where it closes, so each site appears once")
{
    perf::setEnabled(true);
    static perf::Site a("ReportTest::loopA", nullptr);
    static perf::Site b("ReportTest::loopB", nullptr);
    {
        // a first looks outermost
        const perf::Scope aScope(a);
    }
    {
        // b's caller is a
        const perf::Scope aScope(a);
        const perf::Scope bScope(b);
    }
    {
        // a, known only as outermost, takes b as its real caller: a -> b -> a
        const perf::Scope bScope(b);
        const perf::Scope aScope(a);
    }
    REQUIRE(a.caller() == &b);
    REQUIRE(b.caller() == &a);

    const auto report = perf::readReport();
    REQUIRE(count(report, a) == 1);
    REQUIRE(count(report, b) == 1);
}

namespace
{
    std::vector<std::string> lines(const std::string& text)
    {
        std::vector<std::string> out;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            out.push_back(line);
        }
        return out;
    }

    perf::Histogram::Snapshot durations(const std::initializer_list<std::chrono::nanoseconds> samples)
    {
        perf::Histogram histogram;
        for (const auto sample : samples) {
            histogram.record(sample);
        }
        return histogram.drain();
    }
}

TEST_CASE("Perf report text: a header, then one line per thread and per site, indented under its caller")
{
    const perf::Site root("TextTest::onFrame", nullptr);
    const perf::Site child("TextTest::onFrame", "child");
    perf::Report::Node childNode{ &child, { durations({ 2ms, 2ms }), 0 }, {} };
    perf::Report::Node rootNode{ &root, { durations({ 3ms, 3ms }), childNode.stats.durations.sumNs }, { childNode } };
    perf::Report report;
    report.window = 10s;
    report.frames = 2;
    report.threads.push_back({ std::this_thread::get_id(), true, { rootNode } });

    const auto text = perf::formatReport(report);
    REQUIRE(text.ends_with('\n'));
    const auto rows = lines(text);
    REQUIRE(rows.size() == 5);
    REQUIRE(rows[0] == "perf: 10.0s window, 2 frames (0.2 fps), times in ms");
    REQUIRE(rows[1].starts_with("site "));
    REQUIRE(rows[2].starts_with("game thread "));
    REQUIRE(rows[3].starts_with("  TextTest::onFrame "));
    REQUIRE(rows[4].starts_with("    child "));
    // the numbers line up under the header
    REQUIRE(rows[3].size() == rows[1].size());
    REQUIRE(rows[4].size() == rows[1].size());
    // avg 3 ms, self (3 - 2) ms, and one call per frame
    REQUIRE(rows[3].find("   3.000") != std::string::npos);
    REQUIRE(rows[3].find("   1.000") != std::string::npos);
    REQUIRE(rows[3].ends_with("   1.00"));
    REQUIRE(rows[4].find("   2.000") != std::string::npos);
}

TEST_CASE("Perf report text: a site with several callers is marked, with a note at the end")
{
    perf::setEnabled(true);
    static perf::Site first("TextTest::first", nullptr);
    static perf::Site second("TextTest::second", nullptr);
    static perf::Site shared("TextTest::shared", nullptr);
    {
        const perf::Scope a(first);
        const perf::Scope b(shared);
    }
    {
        const perf::Scope a(second);
        const perf::Scope b(shared);
    }
    REQUIRE(shared.hasMultipleCallers());

    perf::Report report;
    report.window = 1s;
    report.threads.push_back({ std::this_thread::get_id(), false, { { &shared, { durations({ 1ms }), 0 }, {} } } });
    const auto rows = lines(perf::formatReport(report));
    REQUIRE(rows[2].starts_with("thread "));
    REQUIRE(rows[3].starts_with("  TextTest::shared * "));
    // without frames there is no calls-per-frame
    REQUIRE(rows[3].ends_with("        -"));
    REQUIRE(rows.back().starts_with("* "));
}

TEST_CASE("Perf report text: with nothing recorded it says so")
{
    perf::Report report;
    report.window = 1s;
    const auto rows = lines(perf::formatReport(report));
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[1] == "no site has recorded anything since the last reset");
}
