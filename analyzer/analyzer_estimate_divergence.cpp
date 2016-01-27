#include "analyzer_estimate_divergence.h"

#include "analyzer_util.h"

bool Analyzer_Estimate_Divergence::configure(INIReader *config) {
    if (!Analyzer::configure(config)) {
        return false;
    }
    _delta_warn = config->GetReal("loganalyzer", string_format("%s::delta_warn", _config_tag().c_str()), default_delta_warn());
    _delta_fail = config->GetReal("loganalyzer", string_format("%s::delta_fail", _config_tag().c_str()), default_delta_fail());
    _delta_time_threshold = config->GetReal("loganalyzer", string_format("%s::duration_min", _config_tag().c_str()), default_duration_min());

    return true;
}

void Analyzer_Estimate_Divergence_Result::to_json(Json::Value &root) const
{
    Analyzer_Result_Period::to_json(root);
    root["name"] = _name;
}

void Analyzer_Estimate_Divergence::end_of_log(const uint32_t packet_count UNUSED)
{
    auto next = _result.begin();
    while (next != _result.end()) {
        auto current = next;
        next++;
        if ((*current).second != NULL) {
            close_result((*current).first);
        }
    }
}

void Analyzer_Estimate_Divergence::update_result_set_status(Analyzer_Estimate_Divergence_Result *result)
{
    if (fabs(result->max_delta()) >= delta_fail()) {
        result->set_status(analyzer_status_fail);
    } else if (fabs(result->max_delta()) >= delta_warn()) {
        result->set_status(analyzer_status_warn);
    }
 }

void Analyzer_Estimate_Divergence::update_result(std::string name, double delta)
{
    Analyzer_Estimate_Divergence_Result *result = _result[name];
    if (fabs(delta) > fabs(result->max_delta())) {
        result->set_max_delta(delta);

        update_result_set_status(result);

        if (result->status() == analyzer_status_fail) {
            result->set_delta_threshold(delta_fail());
            result->set_severity_score(20);
        } else if (result->status() == analyzer_status_warn) {
            result->set_delta_threshold(delta_warn());
            result->set_severity_score(10);
        }
    }
}

void Analyzer_Estimate_Divergence::close_result_add_evidence(Analyzer_Estimate_Divergence_Result *result)
{
    result->add_evidence(string_format("max-delta=%f metres", result->max_delta()));
    result->add_evidence(string_format("delta-threshold=%f metres", result->delta_threshold()));
    result->add_evidence(string_format("delta-time-threshold=%f seconds", delta_time_threshold() / 1000000.0f));
}

void Analyzer_Estimate_Divergence::close_result(std::string name)
{
    _result[name]->set_T_stop(_vehicle->T());
    if (_result[name]->duration() < delta_time_threshold()) {
        delete _result[name];
        _result[name] = NULL;
        return;
    }

    close_result_add_evidence(_result[name]);

    add_result(_result[name]);
    _result[name] = NULL;
}

void *xmalloc(size_t size)
{
    void *ret = malloc(size);
    if (ret == NULL) {
        perror("malloc");
        abort();
    }
    return ret;
}

char *Analyzer_Estimate_Divergence::lc_stdstring(const std::string string) const
{
    size_t length = string.size();
    char *ret = (char*)xmalloc(length+1);
    string.copy(ret, length);
    for (size_t i=0; i<=length; i++) {
        ret[i] = tolower(ret[i]);
    }
    ret[length] = '\0';
    return ret;
}
