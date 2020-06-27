//
// Created by Chunel on 2020/6/20.
// 包含同步调用和异步调用的多线程demo
//

#include <time.h>
#include <functional>
#include <future>
#include "../CaissDemoInclude.h"

//const static vector<string> WORDS = {"this", "is", "an", "open", "source", "project", "and", "hope", "it", "will", "be", "useful", "for", "you", "best", "wishes"};

const static vector<string> WORDS = {"111", "211", "311", "411", "511", "611", "711", "811", "911", "1011", "1111", "1211", "1311", "1411", "1511", "1611"};

static atomic<int> cnt(0);
static int cnt2 = 0;
void STDCALL searchCallbackFunc(CAISS_LIST_STRING& words, CAISS_LIST_FLOAT& distances, const void *params) {
    for (const auto& word : words) {
        cout << word << " ";
    }
    cout << " [" << cnt2++ << "]" << endl;
}


/**
 * 异步多线程处理
 * @return
 */
int demo_asyncMultiThreadSearch() {
    CAISS_FUNCTION_BEGIN
    printf("[caiss] enter demo_asyncMultiThreadSearch function ... \n");

    vector<void *> hdlsVec;
    for (int i = 0; i < max_thread_num_ ; i++) {
        void *handle = nullptr;
        ret = CAISS_CreateHandle(&handle);
        CAISS_FUNCTION_CHECK_STATUS
        hdlsVec.push_back(handle);    // 多个handle组成的vector
        ret = CAISS_Init(handle, CAISS_MODE_PROCESS, dist_type_, dim_, model_path_, dist_func_);
        CAISS_FUNCTION_CHECK_STATUS
    }

    int times = SEARCH_TIMES;
    while (times--) {
        for (int i = 0; i < hdlsVec.size(); i++) {
            int num = (int)(rand() + i) % (int)WORDS.size();
            /* 在异步模式下，train，search等函数，不阻塞。但是会随着 */
            //ret = CAISS_Search(hdlsVec[i], (void *)(WORDS[num]).c_str(), search_type_, top_k_);

            ret = CAISS_Search(hdlsVec[i], (void *)(WORDS[num]).c_str(), search_type_, top_k_, searchCallbackFunc, (void *)&i);
            CAISS_FUNCTION_CHECK_STATUS
        }
    }

    int stop = 0;
    cout << "[caiss] input finished ..." << endl;
    cin >> stop;    // 外部等待所有计算结束后，再结束流程

    for (auto &t : hdlsVec) {
        ret = CAISS_destroyHandle(t);
        CAISS_FUNCTION_CHECK_STATUS
    }

    CAISS_FUNCTION_END
}


int syncSearch(void *handle) {
    CAISS_FUNCTION_BEGIN
    int times = SEARCH_TIMES;
    while (times--) {
        // 查询10000次，结束之后正常退出
        // 由于样本原因，可能会出现，输入的词语在模型中无法查到的问题。这种情况会返回非0的值
        int i = (int)rand() % (int)WORDS.size();
        ret = CAISS_Search(handle, (void *)(WORDS[i]).c_str(), search_type_, top_k_, searchCallbackFunc, nullptr);
        CAISS_FUNCTION_CHECK_STATUS

        unsigned int size = 0;
        ret = CAISS_getResultSize(handle, size);
        CAISS_FUNCTION_CHECK_STATUS

        char *result = new char[size + 1];
        memset(result, 0, size + 1);
        ret = CAISS_getResult(handle, result, size);
        CAISS_FUNCTION_CHECK_STATUS
        std::cout << result << std::endl;
        delete [] result;
    }

    CAISS_FUNCTION_END
}


/**
 * 同步查询功能，上层开多个线程，每个线程使用不同的句柄来进行查询
 * @return
 */
int demo_syncMultiThreadSearch() {
    CAISS_FUNCTION_BEGIN

    vector<void *> hdlsVec;
    vector<std::future<int>> futVec;
    for (int i = 0; i < max_thread_num_ ; i++) {
        void *handle = nullptr;
        ret = CAISS_CreateHandle(&handle);
        CAISS_FUNCTION_CHECK_STATUS
        hdlsVec.push_back(handle);    // 多个handle组成的vector
        ret = CAISS_Init(handle, CAISS_MODE_PROCESS, dist_type_, dim_, model_path_, dist_func_);
        CAISS_FUNCTION_CHECK_STATUS
    }

    auto start = clock();
    for (auto &handle : hdlsVec) {
        std::future<int> fut = std::async(std::launch::async, syncSearch, handle);     // 在同步模式下，上层开辟线程去做多次查询的功能
        futVec.push_back(std::move(fut));
    }

    for (auto &fut : futVec) {
        ret = fut.get();
        CAISS_FUNCTION_CHECK_STATUS
    }

    printf("[caiss] [%d] thread process [%d] times query, cost [%d] ms. \n", max_thread_num_, SEARCH_TIMES, (int)(clock() - start) / 1000);

    for (auto &handle : hdlsVec) {
        ret = CAISS_destroyHandle(handle);
        CAISS_FUNCTION_CHECK_STATUS
    }

    CAISS_FUNCTION_END
}