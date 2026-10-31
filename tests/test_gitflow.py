"""Behavior checks for PR routing and non-destructive settings reconciliation."""

import unittest

from scripts.check_gitflow import validate_route
from scripts.configure_github import load_configuration, matches


class GitFlowTests(unittest.TestCase):
    def test_normal_work_targets_dev(self):
        for head in ("feature/grid", "codex/chore/project-initialization", "bugfix/selection", "dependabot/github_actions/checkout-5"):
            with self.subTest(head=head):
                self.assertIsNone(validate_route("dev", head))
                self.assertIsNotNone(validate_route("main", head))

    def test_release_and_hotfix_reach_both_long_lived_branches(self):
        for head in ("release/0.1.0", "hotfix/0.1.1"):
            for base in ("main", "dev"):
                with self.subTest(base=base, head=head):
                    self.assertIsNone(validate_route(base, head))

    def test_release_from_fork_cannot_target_stable(self):
        self.assertIsNotNone(validate_route("main", "release/0.1.0", False))
        self.assertIsNotNone(validate_route("dev", "hotfix/0.1.1", False))

    def test_release_names_must_match_protected_stabilization_patterns(self):
        self.assertIsNotNone(validate_route("main", "codex/release/0.1.0"))
        self.assertIsNotNone(validate_route("main", "codex/hotfix/0.1.1"))

    def test_fork_contributions_can_target_dev(self):
        self.assertIsNone(validate_route("dev", "feature/grid", False))

    def test_main_back_merge_preserves_ancestry(self):
        self.assertIsNone(validate_route("dev", "main"))
        self.assertIsNotNone(validate_route("main", "dev"))
        self.assertIsNotNone(validate_route("dev", "main", False))

    def test_release_stabilization_does_not_accept_new_features(self):
        self.assertIsNone(validate_route("release/0.1.0", "bugfix/preview"))
        self.assertIsNotNone(validate_route("release/0.1.0", "feature/new-editor"))

    def test_unknown_target_and_unnamed_work_are_rejected(self):
        for base, head in (("other", "feature/grid"), ("dev", "feature/"), ("dev", "misc")):
            with self.subTest(base=base, head=head):
                self.assertIsNotNone(validate_route(base, head))

    def test_legacy_develop_target_is_accepted_until_the_rename_lands(self):
        # The rename PR and any PR still open against `develop` must keep passing.
        for head in ("chore/rename-develop-to-dev", "feature/grid"):
            with self.subTest(head=head):
                self.assertIsNone(validate_route("develop", head))
                self.assertIsNone(validate_route("develop", head, False))
        self.assertIsNone(validate_route("develop", "release/0.1.0"))
        self.assertIsNone(validate_route("develop", "main"))
        self.assertIsNotNone(validate_route("develop", "hotfix/0.1.1", False))


class ConfigurationTests(unittest.TestCase):
    def test_server_defaults_and_rule_order_are_accepted(self):
        expected = {"rules": [{"type": "deletion"}, {"type": "non_fast_forward"}]}
        actual = {"id": 1, "rules": [{"type": "non_fast_forward"}, {"type": "deletion"}]}
        self.assertTrue(matches(expected, actual))

    def test_missing_or_weakened_protection_is_rejected(self):
        self.assertFalse(matches({"enforcement": "active"}, {"enforcement": "disabled"}))
        self.assertFalse(matches({"bypass_actors": []}, {"bypass_actors": [{"actor_id": 5}]}))
        self.assertFalse(matches({"required_review_thread_resolution": True}, {}))

    def test_list_items_cannot_reuse_the_same_server_entry(self):
        self.assertFalse(matches(["main", "main"], ["main", "dev"]))

    def test_checked_in_rules_preserve_gitflow_and_single_maintainer_access(self):
        settings, rules = load_configuration()
        self.assertTrue(settings["allow_merge_commit"])
        self.assertFalse(settings["allow_squash_merge"])
        self.assertFalse(settings["allow_rebase_merge"])
        self.assertFalse(settings["delete_branch_on_merge"])
        for rule in rules:
            self.assertEqual(rule["enforcement"], "active")
            self.assertEqual(rule["bypass_actors"], [])
            by_type = {item["type"]: item for item in rule["rules"]}
            self.assertIn("non_fast_forward", by_type)
            self.assertNotIn("required_linear_history", by_type)
            if rule["name"] in {"gitflow-main", "gitflow-dev"}:
                self.assertIn("deletion", by_type)
                pr = by_type["pull_request"]["parameters"]
                self.assertEqual(pr["allowed_merge_methods"], ["merge"])
                self.assertEqual(pr["required_approving_review_count"], 0)
                status = by_type["required_status_checks"]["parameters"]
                self.assertTrue(status["strict_required_status_checks_policy"])
                self.assertEqual(status["required_status_checks"][0]["context"], "Required checks")


if __name__ == "__main__":
    unittest.main()
