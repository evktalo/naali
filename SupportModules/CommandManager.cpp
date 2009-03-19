// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"

#include <boost/tokenizer.hpp>
#include "CommandManager.h"
#include "ConsoleModule.h"

bool nocase_compare(const std::string &lhs, const std::string &rhs)
{
    return boost::algorithm::iequals(lhs, rhs);
}

namespace Console
{
    typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
    typedef boost::tokenizer< boost::escaped_list_separator<char> > escape_tokenizer;

    CommandManager::CommandManager(Foundation::ModuleInterface *parent, ConsoleServiceInterface *console) : 
    Console::ConsoleCommandServiceInterface()
        , parent_ (static_cast< ConsoleModule* >(parent))
        , console_(console)
    {
        RegisterCommand("Help", "Display available commands", Console::Bind(this, &CommandManager::Help));
        RegisterCommand("Exit", "Exit application", Console::Bind(this, &CommandManager::Exit));
    }

    CommandManager::~CommandManager()
    {
    }

    void CommandManager::Update()
    {
        while (commandlines_.empty() == false)
        {
            std::string command_line;
            {
                Core::MutexLock lock(commandlines_mutex_);
                command_line = commandlines_.front();
                commandlines_.pop();
            }

            ExecuteCommand(command_line);
        }
    }

    void CommandManager::QueueCommand(const std::string &commandline)
    {
        Core::MutexLock lock(commandlines_mutex_);
        commandlines_.push(commandline);
    }

    void CommandManager::RegisterCommand(const Console::Command &command)
    {
        Core::RecursiveMutexLock lock(commands_mutex_);

        if (commands_.find(command.name_) != commands_.end())
        {
            ConsoleModule::LogError("Command " + command.name_ + " already registered.");
            return;
        }
    
        std::string name = command.name_;
        boost::to_lower(name);
        commands_[name] = command;
    }

    boost::optional<CommandResult> CommandManager::Poll(const std::string &command)
    {
        std::string command_l = command;
        boost::to_lower(command_l);

        Core::RecursiveMutexLock lock(commands_mutex_);
        
        CommandParamMap::iterator it = delayed_commands_.find(command_l);
        if (it != delayed_commands_.end())
        {
            CommandResult result = ExecuteCommandAlways(it->first, it->second, true); // Core::MutexLock recursive, no deadlock
            delayed_commands_.erase(it);

            return boost::optional<CommandResult>(result);
        }
        return boost::optional<CommandResult>();
    }

    Console::CommandResult CommandManager::ExecuteCommand(const std::string &commandline)
    {
        std::string cl = commandline;

        // separate command and parameters
        boost::char_separator<char> sep("([{)]}");
        tokenizer commandline_tok(cl, sep);

        
        std::string command;
        std::string param_line;
        Core::StringVector params;

        tokenizer::iterator it = commandline_tok.begin();
        if (it == commandline_tok.end())
        {
            console_->Print("Failed to parse malformed command line: " + commandline);
            Console::CommandResult result = { false, "" };
            return result;
        }

        command = *it;
        boost::trim(command);

        ++it;
        if (it != commandline_tok.end())
        {
            param_line = *it;

            // Separate parameters and push to vector
            
            boost::escaped_list_separator<char> param_sep;
            escape_tokenizer param_tok(param_line);

            for (escape_tokenizer::iterator it = param_tok.begin() ;
                 it != param_tok.end() ; 
                 ++it)
            {
                std::string param = *it;
                boost::trim(param);
                params.push_back(param);
            }
        }

        return ExecuteCommand(command, params);
    }

    Console::CommandResult CommandManager::ExecuteCommandAlways(const std::string &name, const Core::StringVector &params, bool always)
    {
        std::string low_name = name;
        boost::to_lower(low_name);
        
        Console::CallbackPtr callback;
        bool delayed = false;
        {
            Core::RecursiveMutexLock lock(commands_mutex_);
            CommandMap::const_iterator iter = commands_.find(low_name);
            if (iter == commands_.end())
            {
                console_->Print("Command: " + name + " not found. Type 'help' for list of available commands.");
                Console::CommandResult result = { false, "" };
                return result;
            }
            callback = iter->second.callback_;
            if (!always && iter->second.delayed_)
            {
                delayed_commands_[iter->first] = params;
                return ResultDelayed();
            }
        }

        Console::CommandResult result = (*callback)(params);
        if (result.why_.empty() == false)
        {
            if (result.success_ == false)
            {
                console_->Print("Error: " + result.why_);
            } else
            {
                console_->Print(result.why_);
            }
        }
        return result;
    }

    Console::CommandResult CommandManager::Help(const Core::StringVector &params)
    {
        if (params.empty())
        {
            console_->Print("Available commands with descriptions:");
        }
        bool success = false;

        {   
            Core::RecursiveMutexLock lock(commands_mutex_);
            CommandMap::const_iterator it = commands_.begin();
            for ( ; it != commands_.end() ; ++it)
            {
                if (params.empty() || std::find_if(params.begin(), params.end(), boost::bind( &nocase_compare, _1, it->second.name_ )) != params.end())
                {
                    console_->Print(it->second.name_ + " - " + it->second.description_);
                    success = true;
                }
            }
        }

        if (!success)
        {
            assert (params.empty() == false);
            return Console::ResultFailure("Unknown command " + params[0]);
        }

        if (params.empty())
        {
            console_->Print("For help with specific command, type help(command).");
        }

        return Console::ResultSuccess();
    }

    Console::CommandResult CommandManager::Exit(const Core::StringVector &params)
    {
        console_->Print("Exiting");
        parent_->GetFramework()->Exit();

        return Console::ResultSuccess();
    }
}

