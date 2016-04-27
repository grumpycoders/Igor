#include <TaskMan.h>
#include <Task.h>

#include "IgorAsyncActions.h"
#include "IgorLocalSession.h"

class IgorAsyncAction {
public:
	virtual void Do() = 0;
};

class IgorAsyncActionStopper : public IgorAsyncAction {
public:
	virtual void Do() { IAssert(false, "shouldn't get run"); }
};

class IgorAsyncWorker : public Balau::Task {
public:
	void Do() {
		while (true) {
			IgorAsyncAction * action = m_queue.pop();
			if (dynamic_cast<IgorAsyncActionStopper *>(action)) {
				delete action;
				return;
			}
			action->Do();
			delete action;
		}
	}
	void push(IgorAsyncAction * action) {
		m_queue.push(action);
	}
private:
	Balau::TQueue<IgorAsyncAction> m_queue;
	virtual const char * getName() const { return "IgorAsyncAction"; }
};

static IgorAsyncWorker * igorAsyncWorker;

void startIgorAsyncWorker() {
	Balau::TaskMan::registerTask(igorAsyncWorker = new IgorAsyncWorker());
}

void stopIgorAsyncWorker() {
	igorAsyncWorker->push(new IgorAsyncActionStopper());
}

class IgorAsyncLoader : public IgorAsyncAction {
public:
	IgorAsyncLoader(std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> * results, Balau::Queue<void> * cv, const char * filename) : m_results(results), m_cv(cv), m_filename(filename) { }
	virtual void Do() {
		*m_results = IgorLocalSession::loadBinary(m_filename);
		m_cv->push(NULL);
	}
private:
	std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> * m_results;
	Balau::Queue<void> * m_cv;
	const char * m_filename;
};

std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> IgorAsyncLoadBinary(const char * filename) {
	std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> results;
	Balau::Queue<void> cv;
	igorAsyncWorker->push(new IgorAsyncLoader(&results, &cv, filename));
	cv.pop();
	return results;
}

